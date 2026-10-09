// ============================================================================
//  T3 信号参数估计 —— 本地全模式自检程序
//
//  直接运行（无需命令行参数、无需标准输入）即可测试全部 5 种模式：
//  每个模式内置多组不同参数的合成信号，叠加高斯噪声后交给 SignalIdentifier
//  估计，再按题目阈值 T_j = k * s_j + eta（k=4, eta=1e-8）逐项判定是否达标。
//
//  用法: ./t3_test
//  退出码: 0 = 全部通过；1 = 存在失败项
// ============================================================================
#include "signal_identifier.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <random>
#include <vector>

namespace {

constexpr double kPi  = 3.14159265358979323846;
constexpr double kK   = 4.0;      // 阈值倍数
constexpr double kEta = 1e-8;     // 阈值下限

// ---------------------------------------------------------------------------
//  一组测试：模式 + 真值参数 + 采样/噪声配置
//  truth 按接口的六项顺序存放；valid 为有效参数个数。
// ---------------------------------------------------------------------------
struct TestCase {
    const char* title;
    SignalMode mode;
    std::array<double, 6> truth;
    int valid;
    double sampleHz;
    double duration;
    double sigma;
    unsigned seed;
};

const char* modeName(SignalMode m)
{
    switch (m) {
        case SignalMode::Constant:      return "Constant";
        case SignalMode::Linear:        return "Linear";
        case SignalMode::Quadratic:     return "Quadratic";
        case SignalMode::Sine:          return "Sine";
        case SignalMode::QuadraticSine: return "QuadraticSine";
    }
    return "?";
}

const char* paramName(SignalMode m, int j)
{
    static const char* table[5][6] = {
        {"c", "",  "",  "",    "",    ""   },
        {"c", "v", "",  "",    "",    ""   },
        {"c", "v", "a", "",    "",    ""   },
        {"b", "A", "f", "phi", "",    ""   },
        {"c", "v", "a", "A",   "f",   "phi"},
    };
    return table[static_cast<int>(m)][j];
}

// 原始信号 s(tau; theta)
double evalSignal(SignalMode m, const std::array<double, 6>& p, double tau)
{
    switch (m) {
        case SignalMode::Constant:
            return p[0];
        case SignalMode::Linear:
            return p[0] + p[1] * tau;
        case SignalMode::Quadratic:
            return p[0] + p[1] * tau + 0.5 * p[2] * tau * tau;
        case SignalMode::Sine:
            return p[0] + p[1] * std::sin(2.0 * kPi * p[2] * tau + p[3]);
        case SignalMode::QuadraticSine:
            return p[0] + p[1] * tau + 0.5 * p[2] * tau * tau
                 + p[3] * std::sin(2.0 * kPi * p[4] * tau + p[5]);
    }
    return 0.0;
}

// 观测雅可比行 J[j] = ∂s/∂θ_j（未使用项为 0）
void jacobianRow(SignalMode m, const std::array<double, 6>& p, double tau, double* J)
{
    for (int i = 0; i < 6; ++i) J[i] = 0.0;
    switch (m) {
        case SignalMode::Constant:
            J[0] = 1.0;
            break;
        case SignalMode::Linear:
            J[0] = 1.0; J[1] = tau;
            break;
        case SignalMode::Quadratic:
            J[0] = 1.0; J[1] = tau; J[2] = 0.5 * tau * tau;
            break;
        case SignalMode::Sine: {
            const double w = 2.0 * kPi * p[2] * tau + p[3];
            J[0] = 1.0;
            J[1] = std::sin(w);
            J[2] = p[1] * std::cos(w) * 2.0 * kPi * tau;
            J[3] = p[1] * std::cos(w);
            break;
        }
        case SignalMode::QuadraticSine: {
            const double w = 2.0 * kPi * p[4] * tau + p[5];
            J[0] = 1.0;
            J[1] = tau;
            J[2] = 0.5 * tau * tau;
            J[3] = std::sin(w);
            J[4] = p[3] * std::cos(w) * 2.0 * kPi * tau;
            J[5] = p[3] * std::cos(w);
            break;
        }
    }
}

// 角度归一化到 [-pi, pi)
double wrapPi(double a)
{
    const double twoPi = 2.0 * kPi;
    a = std::fmod(a, twoPi);
    if (a >= kPi)      a -= twoPi;
    else if (a < -kPi) a += twoPi;
    return a;
}

// 相位项用圆周误差，其余用绝对误差
double paramError(SignalMode m, int j, double est, double truth)
{
    const bool isPhase =
        (m == SignalMode::Sine && j == 3) ||
        (m == SignalMode::QuadraticSine && j == 5);
    return isPhase ? std::fabs(wrapPi(est - truth)) : std::fabs(est - truth);
}

// 求 n×n 矩阵的逆的对角线（部分主元高斯-约当）；奇异返回 false
bool inverseDiag(std::vector<double> A, int n, std::vector<double>& diag)
{
    std::vector<double> inv(static_cast<std::size_t>(n) * n, 0.0);
    for (int i = 0; i < n; ++i) inv[static_cast<std::size_t>(i) * n + i] = 1.0;
    for (int col = 0; col < n; ++col) {
        int piv = col;
        double best = std::fabs(A[static_cast<std::size_t>(col) * n + col]);
        for (int r = col + 1; r < n; ++r) {
            const double v = std::fabs(A[static_cast<std::size_t>(r) * n + col]);
            if (v > best) { best = v; piv = r; }
        }
        if (!(best > 0.0) || !std::isfinite(best)) return false;
        if (piv != col) {
            for (int c = 0; c < n; ++c) {
                std::swap(A[static_cast<std::size_t>(col) * n + c],
                          A[static_cast<std::size_t>(piv) * n + c]);
                std::swap(inv[static_cast<std::size_t>(col) * n + c],
                          inv[static_cast<std::size_t>(piv) * n + c]);
            }
        }
        const double d = A[static_cast<std::size_t>(col) * n + col];
        for (int c = 0; c < n; ++c) {
            A[static_cast<std::size_t>(col) * n + c] /= d;
            inv[static_cast<std::size_t>(col) * n + c] /= d;
        }
        for (int r = 0; r < n; ++r) {
            if (r == col) continue;
            const double factor = A[static_cast<std::size_t>(r) * n + col];
            if (factor == 0.0) continue;
            for (int c = 0; c < n; ++c) {
                A[static_cast<std::size_t>(r) * n + c] -=
                    factor * A[static_cast<std::size_t>(col) * n + c];
                inv[static_cast<std::size_t>(r) * n + c] -=
                    factor * inv[static_cast<std::size_t>(col) * n + c];
            }
        }
    }
    diag.assign(static_cast<std::size_t>(n), 0.0);
    for (int i = 0; i < n; ++i) {
        const double v = inv[static_cast<std::size_t>(i) * n + i];
        if (!(v >= 0.0) || !std::isfinite(v)) return false;
        diag[static_cast<std::size_t>(i)] = v;
    }
    return true;
}

// 阈值 T_j = k * sqrt(sigma^2 * (M^-1)_jj) + eta，M = Σ_k J_k J_k^T（真值处）
std::vector<double> thresholds(const TestCase& tc, const std::vector<double>& tau)
{
    const int n = tc.valid;
    std::vector<double> M(static_cast<std::size_t>(n) * n, 0.0);
    double J[6];
    for (double t : tau) {
        jacobianRow(tc.mode, tc.truth, t, J);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                M[static_cast<std::size_t>(i) * n + j] += J[i] * J[j];
    }
    std::vector<double> diag, T(static_cast<std::size_t>(n), kEta);
    if (inverseDiag(M, n, diag))
        for (int i = 0; i < n; ++i)
            T[static_cast<std::size_t>(i)] =
                kK * tc.sigma * std::sqrt(diag[static_cast<std::size_t>(i)]) + kEta;
    return T;
}

// 内置用例：每个模式 3 组不同参数，覆盖正负值、零值与区间端点附近
const std::vector<TestCase>& allCases()
{
    static const std::vector<TestCase> cases = {
        // ---- Constant ----
        {"Constant: c=3",          SignalMode::Constant, { 3.0,  0.0,  0.0, 0.0, 0.0, 0.0}, 1,  50.0, 10.0, 1.5, 101},
        {"Constant: c=-7.5",       SignalMode::Constant, {-7.5,  0.0,  0.0, 0.0, 0.0, 0.0}, 1, 100.0, 10.0, 1.0, 102},
        {"Constant: c=0 (zero)",   SignalMode::Constant, { 0.0,  0.0,  0.0, 0.0, 0.0, 0.0}, 1,  50.0, 10.0, 0.0, 103},
        // ---- Linear ----
        {"Linear: c=3, v=2",       SignalMode::Linear, { 3.0,  2.0,  0.0, 0.0, 0.0, 0.0}, 2, 100.0, 10.0, 1.5, 104},
        {"Linear: c=-4.2, v=-1.3", SignalMode::Linear, {-4.2, -1.3,  0.0, 0.0, 0.0, 0.0}, 2,  50.0, 10.0, 2.0, 105},
        {"Linear: c=0, v=0.7",     SignalMode::Linear, { 0.0,  0.7,  0.0, 0.0, 0.0, 0.0}, 2, 100.0, 10.0, 0.5, 106},
        // ---- Quadratic ----
        {"Quadratic: c=1, v=2, a=0.4",       SignalMode::Quadratic, { 1.0,  2.0,  0.4, 0.0, 0.0, 0.0}, 3,  50.0, 20.0, 5.0, 107},
        {"Quadratic: c=-3, v=-0.5, a=-0.3",  SignalMode::Quadratic, {-3.0, -0.5, -0.3, 0.0, 0.0, 0.0}, 3,  20.0, 20.0, 8.0, 108},
        {"Quadratic: c=5, v=1.5, a=0.2",     SignalMode::Quadratic, { 5.0,  1.5,  0.2, 0.0, 0.0, 0.0}, 3, 100.0, 10.0, 3.0, 109},
        // ---- Sine ----
        {"Sine: b=0.5, A=2, f=0.5, phi=0.3",   SignalMode::Sine, { 0.5, 2.0, 0.5,  0.3, 0.0, 0.0}, 4, 50.0, 40.0, 0.8, 110},
        {"Sine: b=-2, A=3, f=1.2, phi=-1.5",   SignalMode::Sine, {-2.0, 3.0, 1.2, -1.5, 0.0, 0.0}, 4, 50.0, 40.0, 1.5, 111},
        {"Sine: b=8, A=0.6, f=0.2, phi=2.9",   SignalMode::Sine, { 8.0, 0.6, 0.2,  2.9, 0.0, 0.0}, 4, 50.0, 40.0, 0.4, 112},
        // ---- QuadraticSine ----
        {"QuadSine: c=1, v=0.5, a=0.3, A=2, f=0.5, phi=0.3",   SignalMode::QuadraticSine, { 1.0,  0.5,  0.3, 2.0, 0.5,  0.3}, 6, 100.0, 30.0, 1.2, 113},
        {"QuadSine: c=-5, v=-1, a=-0.4, A=4, f=1.5, phi=-0.8", SignalMode::QuadraticSine, {-5.0, -1.0, -0.4, 4.0, 1.5, -0.8}, 6, 100.0, 30.0, 2.4, 114},
        {"QuadSine: c=2, v=0.2, a=0.25, A=1, f=1, phi=3.0",    SignalMode::QuadraticSine, { 2.0,  0.2,  0.25, 1.0, 1.0,  3.0}, 6, 100.0, 30.0, 0.5, 115},
    };
    return cases;
}

}  // namespace

int main()
{
    std::printf("============== T3 信号参数估计 · 全模式自检 ==============\n");
    std::printf("判定阈值: T_j = k*s_j + eta   (k=%g, eta=%g)\n", kK, kEta);

    int total = 0, passed = 0;
    for (const TestCase& tc : allCases()) {
        // 1) 合成观测：均匀采样 + 独立高斯噪声
        const long N = static_cast<long>(tc.duration * tc.sampleHz + 0.5) + 1;
        std::vector<double> tau(static_cast<std::size_t>(N)), y(static_cast<std::size_t>(N));
        std::mt19937 rng(tc.seed);
        std::normal_distribution<double> noise(0.0, tc.sigma > 0.0 ? tc.sigma : 1.0);
        for (long k = 0; k < N; ++k) {
            const double t = static_cast<double>(k) / tc.sampleHz;
            tau[static_cast<std::size_t>(k)] = t;
            const double nz = (tc.sigma > 0.0) ? noise(rng) : 0.0;
            y[static_cast<std::size_t>(k)] = evalSignal(tc.mode, tc.truth, t) + nz;
        }

        // 2) 逐帧喂入并估计
        SignalIdentifier id(tc.mode, tc.sigma);
        for (long k = 0; k < N; ++k)
            id.update(tau[static_cast<std::size_t>(k)], y[static_cast<std::size_t>(k)]);
        const ParameterEstimate est = id.estimate();

        // 3) 逐项判定
        const std::vector<double> T = thresholds(tc, tau);
        bool ok = true;
        for (int j = 0; j < tc.valid; ++j) {
            const double e = paramError(tc.mode, j,
                                        est.parameters[static_cast<std::size_t>(j)],
                                        tc.truth[static_cast<std::size_t>(j)]);
            if (!(e <= T[static_cast<std::size_t>(j)])) ok = false;
        }
        for (int j = tc.valid; j < 6; ++j)                    // 未使用项必须为 0
            if (est.parameters[static_cast<std::size_t>(j)] != 0.0) ok = false;

        ++total;
        if (ok) ++passed;

        // 4) 打印结果
        std::printf("\n[%s] %s   (%s, %ld 帧, %.0f Hz, %.0f s, sigma=%.4g)\n",
                    ok ? "PASS" : "FAIL", tc.title, modeName(tc.mode), N,
                    tc.sampleHz, tc.duration, tc.sigma);
        std::printf("   参数   真值             估计             误差             阈值\n");
        for (int j = 0; j < tc.valid; ++j) {
            const double e = paramError(tc.mode, j,
                                        est.parameters[static_cast<std::size_t>(j)],
                                        tc.truth[static_cast<std::size_t>(j)]);
            std::printf("   %-5s %15.8g  %15.8g  %14.5g  %14.5g   %s\n",
                        paramName(tc.mode, j), tc.truth[static_cast<std::size_t>(j)],
                        est.parameters[static_cast<std::size_t>(j)], e,
                        T[static_cast<std::size_t>(j)],
                        (e <= T[static_cast<std::size_t>(j)]) ? "OK" : "FAIL");
        }
    }

    std::printf("\n============== 汇总: %d/%d 通过 ==============\n", passed, total);
    return (passed == total) ? 0 : 1;
}
