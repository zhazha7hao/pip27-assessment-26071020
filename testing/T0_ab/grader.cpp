#include "ab.hpp"
#include "resource_runner.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

namespace {

struct TestPoint {
    int a;
    int b;
};

constexpr int kMaxInput = 10000;
constexpr int kTestCount = 10;
constexpr std::uint32_t kDefaultSeed = 20271001U;
constexpr assessment::Limits kLimits{
    std::chrono::milliseconds(1000), 64ULL * 1024 * 1024, 1, 1};

std::uint32_t readSeed() {
    const char* value = std::getenv("PIP27_T0_AB_SEED");
    if (value == nullptr || *value == '\0') {
        return kDefaultSeed;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (*end != '\0' || parsed > UINT32_MAX) {
        std::cerr << "Invalid PIP27_T0_AB_SEED; using default seed\n";
        return kDefaultSeed;
    }
    return static_cast<std::uint32_t>(parsed);
}

std::array<TestPoint, kTestCount> makeTestPoints(std::uint32_t seed) {
    std::array<TestPoint, kTestCount> points{};
    points[0] = {0, 0};
    points[1] = {0, kMaxInput};
    points[2] = {kMaxInput, 0};
    points[3] = {kMaxInput, kMaxInput};

    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(0, kMaxInput);
    for (std::size_t i = 4; i < points.size(); ++i) {
        points[i] = {distribution(generator), distribution(generator)};
    }
    return points;
}

const char* verdictName(assessment::Verdict verdict) {
    switch (verdict) {
        case assessment::Verdict::Passed:
            return "PASS";
        case assessment::Verdict::WrongAnswer:
            return "WRONG_ANSWER";
        case assessment::Verdict::TimeLimit:
            return "TIME_LIMIT";
        case assessment::Verdict::CpuLimit:
            return "CPU_LIMIT";
        case assessment::Verdict::MemoryLimit:
            return "MEMORY_LIMIT";
        case assessment::Verdict::RuntimeError:
            return "RUNTIME_ERROR";
        case assessment::Verdict::SetupError:
            return "GRADER_SETUP";
    }
    return "UNKNOWN";
}

void printPoint(std::size_t number, const assessment::CaseResult& result) {
    std::cout << std::right << std::setw(3) << number << "  " << std::left
              << std::setw(16) << verdictName(result.verdict) << std::right
              << std::setw(15)
              << std::chrono::duration<double, std::milli>(
                     result.elapsed_time)
                     .count();
    if (result.peak_rss_kib) {
        std::cout << std::setw(15) << *result.peak_rss_kib;
    } else {
        std::cout << std::setw(15) << "--";
    }
    std::cout << '\n';
}

}  // namespace

int main() {
    const std::uint32_t seed = readSeed();
    const auto points = makeTestPoints(seed);
    int passed = 0;

    std::cout << "PIP2027 / T0 A+B 测评报告\n"
              << "测试点: " << points.size() << '\n'
              << "运行时间限制: " << kLimits.wall_time.count() << " ms\n"
              << "程序内存限制: "
              << kLimits.address_space_bytes / (1024 * 1024)
              << " MiB\n"
              << "程序 CPU 数量限制: " << kLimits.cpu_count << " 核\n\n";
    std::cout
              << std::right << std::setw(3) << "No" << "  " << std::left
              << std::setw(16) << "Verdict" << std::right << std::setw(15)
              << "Runtime(ms)" << std::setw(15) << "PeakRSS(KiB)" << '\n'
              << std::string(51, '-') << '\n'
              << std::fixed << std::setprecision(3);

    for (std::size_t index = 0; index < points.size(); ++index) {
        // 构造、求和及结果检查都在同一个受限测试点中执行。
        const auto result = assessment::runCase(kLimits, [&] {
            const Solver solver(points[index].a, points[index].b);
            return solver.sum() == points[index].a + points[index].b;
        });
        if (result.verdict == assessment::Verdict::Passed) {
            ++passed;
        }
        if (result.verdict == assessment::Verdict::SetupError) {
            // 测评机自身出错时不能把未完成的测试点算成考生失分。
            std::cerr << "测试点 " << index + 1 << ": 测评机运行失败\n";
            return 2;
        }
        printPoint(index + 1, result);
    }

    std::cout << std::string(51, '-') << "\n测评结果\n";
    std::cout << passed << '/' << points.size() << '\n';
    return passed == kTestCount ? 0 : 1;
}
