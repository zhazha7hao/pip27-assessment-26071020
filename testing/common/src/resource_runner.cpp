#include <resource_runner.hpp>

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <thread>

#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace assessment {
namespace {

using Clock = std::chrono::steady_clock;

// 管道只传输固定长度的测评结果；没有结果的正常退出不能算通过。
struct WireResult {
    std::int32_t verdict;
    std::int64_t completed_nanoseconds;
};

[[noreturn]] void reportAndExit(int fd, Verdict verdict) {
    // 子进程记录完成时刻，避免父进程的轮询间隔影响正常完成点的耗时。
    const WireResult result{static_cast<std::int32_t>(verdict),
                            std::chrono::duration_cast<
                                std::chrono::nanoseconds>(
                                Clock::now().time_since_epoch())
                                .count()};
    ssize_t written;
    do {
        written = write(fd, &result, sizeof(result));
    } while (written < 0 && errno == EINTR);
    _exit(written == sizeof(result) ? 0 : 129);
}

bool applyProcessLimits(const Limits& limits) {
    // 崩溃的提交代码不应在测评工作目录留下 core 文件。
    const rlimit no_core{0, 0};
    if (setrlimit(RLIMIT_CORE, &no_core) != 0) {
        return false;
    }

    const rlim_t memory_bytes = static_cast<rlim_t>(limits.address_space_bytes);
    const rlimit memory{memory_bytes, memory_bytes};
    if (setrlimit(RLIMIT_AS, &memory) != 0) {
        return false;
    }

    // RLIMIT_CPU 只接受整秒，作为 CPU 时间兜底；软限制先发 SIGXCPU。
    const rlim_t cpu_seconds = static_cast<rlim_t>(limits.cpu_seconds);
    const rlimit cpu{cpu_seconds, cpu_seconds + 1};
    return setrlimit(RLIMIT_CPU, &cpu) == 0;
}

bool limitAvailableCpus(unsigned int cpu_count) {
    cpu_set_t available;
    if (sched_getaffinity(0, sizeof(available), &available) != 0) {
        return false;
    }
    cpu_set_t selected;
    CPU_ZERO(&selected);
    unsigned int selected_count = 0;
    for (int cpu = 0; cpu < CPU_SETSIZE && selected_count < cpu_count; ++cpu) {
        if (CPU_ISSET(cpu, &available)) {
            CPU_SET(cpu, &selected);
            ++selected_count;
        }
    }
    if (selected_count != cpu_count ||
        sched_setaffinity(0, sizeof(selected), &selected) != 0) {
        return false;
    }

#if defined(__x86_64__)
    constexpr std::uint32_t kArchitecture = AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
    constexpr std::uint32_t kArchitecture = AUDIT_ARCH_AARCH64;
#else
#error "CPU affinity guard supports x86_64 and aarch64 only"
#endif
    // 禁止被测代码重新放宽亲和性；过滤器会由 fork/exec 的子进程继承。
    sock_filter filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, kArchitecture, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
#if defined(__x86_64__)
        BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, 0x40000000U, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
#endif
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_sched_setaffinity, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    sock_fprog program{static_cast<unsigned short>(sizeof(filter) /
                                                   sizeof(filter[0])),
                       filter};
    return prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0 &&
           prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0;
}

[[noreturn]] void runChild(int result_fd, const Limits& limits,
                           const std::function<bool()>& check) {
    // 独立进程组使父进程能清理这个测试点派生的普通子进程。
    if (setpgid(0, 0) != 0 || !applyProcessLimits(limits) ||
        !limitAvailableCpus(limits.cpu_count)) {
        reportAndExit(result_fd, Verdict::SetupError);
    }

    struct sigaction cpu_action {};
    cpu_action.sa_handler = SIG_DFL;
    sigemptyset(&cpu_action.sa_mask);
    sigset_t unblocked;
    sigemptyset(&unblocked);
    sigaddset(&unblocked, SIGXCPU);
    if (sigaction(SIGXCPU, &cpu_action, nullptr) != 0 ||
        sigprocmask(SIG_UNBLOCK, &unblocked, nullptr) != 0) {
        reportAndExit(result_fd, Verdict::SetupError);
    }

    try {
        reportAndExit(result_fd,
                      check() ? Verdict::Passed : Verdict::WrongAnswer);
    } catch (const std::bad_alloc&) {
        reportAndExit(result_fd, Verdict::MemoryLimit);
    } catch (...) {
        reportAndExit(result_fd, Verdict::RuntimeError);
    }
}

void terminateProcessGroup(pid_t child) {
    kill(-child, SIGKILL);
    kill(child, SIGKILL);
}

bool reapChild(pid_t child, int* status, rusage* usage) {
    pid_t waited;
    do {
        waited = wait4(child, status, 0, usage);
    } while (waited < 0 && errno == EINTR);
    return waited == child;
}

Verdict decodeVerdict(std::int32_t raw) {
    switch (static_cast<Verdict>(raw)) {
        case Verdict::Passed:
        case Verdict::WrongAnswer:
        case Verdict::TimeLimit:
        case Verdict::CpuLimit:
        case Verdict::MemoryLimit:
        case Verdict::RuntimeError:
        case Verdict::SetupError:
            return static_cast<Verdict>(raw);
    }
    return Verdict::RuntimeError;
}

}  // namespace

CaseResult runCase(const Limits& limits, const std::function<bool()>& check) {
    CaseResult result{Verdict::SetupError,
                      std::chrono::nanoseconds::zero(), std::nullopt, 0};
    if (!check || limits.wall_time.count() <= 0 ||
        limits.address_space_bytes == 0 || limits.cpu_seconds == 0 ||
        limits.cpu_count == 0 ||
        limits.address_space_bytes > std::numeric_limits<rlim_t>::max() ||
        static_cast<std::uint64_t>(limits.cpu_seconds) + 1 >=
            std::numeric_limits<rlim_t>::max()) {
        return result;
    }

    int result_pipe[2];
    if (pipe2(result_pipe, O_CLOEXEC | O_NONBLOCK) != 0) {
        return result;
    }
    const auto start = Clock::now();
    const pid_t child = fork();
    if (child < 0) {
        close(result_pipe[0]);
        close(result_pipe[1]);
        return result;
    }
    if (child == 0) {
        close(result_pipe[0]);
        runChild(result_pipe[1], limits, check);
    }

    close(result_pipe[1]);
    // 与子进程中的 setpgid 配合，覆盖它尚未开始运行的竞态窗口。
    setpgid(child, child);

    int status = 0;
    rusage usage{};
    while (true) {
        const pid_t waited = wait4(child, &status, WNOHANG, &usage);
        if (waited == child) {
            break;
        }
        if (waited < 0 && errno != EINTR) {
            terminateProcessGroup(child);
            reapChild(child, &status, &usage);
            close(result_pipe[0]);
            return result;
        }
        if (Clock::now() - start >= limits.wall_time) {
            terminateProcessGroup(child);
            const bool reaped = reapChild(child, &status, &usage);
            close(result_pipe[0]);
            result.verdict = reaped ? Verdict::TimeLimit : Verdict::SetupError;
            if (reaped) {
                result.peak_rss_kib =
                    static_cast<std::uint64_t>(usage.ru_maxrss);
            }
            result.elapsed_time = Clock::now() - start;
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    result.elapsed_time = Clock::now() - start;
    result.peak_rss_kib = static_cast<std::uint64_t>(usage.ru_maxrss);
    // 主子进程结束后清理同组派生进程，再读取固定长度的测评结果。
    kill(-child, SIGKILL);
    WireResult wire{};
    ssize_t bytes;
    do {
        bytes = read(result_pipe[0], &wire, sizeof(wire));
    } while (bytes < 0 && errno == EINTR);
    close(result_pipe[0]);

    if (WIFSIGNALED(status)) {
        result.signal_number = WTERMSIG(status);
        if (result.signal_number == SIGXCPU) {
            result.verdict = Verdict::CpuLimit;
        } else {
            result.verdict = Verdict::RuntimeError;
        }
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0 &&
               bytes == sizeof(wire)) {
        result.verdict = decodeVerdict(wire.verdict);
        const auto completed = Clock::time_point(
            std::chrono::duration_cast<Clock::duration>(
                std::chrono::nanoseconds(wire.completed_nanoseconds)));
        if (completed >= start) {
            result.elapsed_time = completed - start;
            if (result.elapsed_time >= limits.wall_time &&
                result.verdict != Verdict::SetupError) {
                result.verdict = Verdict::TimeLimit;
            }
        } else {
            result.verdict = Verdict::RuntimeError;
        }
    } else {
        result.verdict = Verdict::RuntimeError;
    }
    return result;
}

}  // namespace assessment
