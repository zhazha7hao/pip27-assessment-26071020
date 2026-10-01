#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

namespace assessment {

// 每个测试点的资源限制；墙钟时间从创建子进程开始计算。
struct Limits {
    std::chrono::milliseconds wall_time;
    std::uint64_t address_space_bytes;
    unsigned int cpu_seconds;
    unsigned int cpu_count;
};

enum class Verdict {
    Passed,
    WrongAnswer,
    TimeLimit,
    CpuLimit,
    MemoryLimit,
    RuntimeError,
    SetupError,
};

struct CaseResult {
    Verdict verdict;
    std::chrono::nanoseconds elapsed_time;
    // wait4 的 ru_maxrss：Linux 上单位为 KiB，统计被测子进程的峰值 RSS。
    std::optional<std::uint64_t> peak_rss_kib;
    int signal_number;
};

// 在独立子进程中执行并检查布尔结果；整个测试点受 wall_time 约束。
CaseResult runCase(const Limits& limits, const std::function<bool()>& check);

}  // namespace assessment
