#include "signal_identifier.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

//静态状态卡尔曼滤波
struct StaticKalmanFilter {
    int n = 0;
    double x[6] = {0.0};
    double p[6][6] = {0.0};

    void init(int n, double prior) {
        this->n = n;
        for (int i = 0; i < n; i++) {
            p[i][i] = prior;
        }
    }
    void update(const double* h, const double r, const double observation) {
        double ph[6];
        double s = r;
        for (int i = 0; i < n; ++i) {
            double cnt = 0.0;
            for (int j = 0; j < n; ++j) cnt += p[i][j] * h[j];
            ph[i] = cnt;
            s += h[i] * cnt;
        }
        if (s == 0) return;
        double k[6];
        for (int i = 0; i < n; ++i) {
            k[i] = ph[i] / s;
        }
        double innov = observation;
        for (int i = 0; i < n; ++i) {
            innov -= h[i] * x[i];
        }
        for (int i = 0; i < n; ++i) {
            x[i] += k[i] * innov;
        }
        double temp1[6][6] = {0.0}, temp2[6][6] = {0.0};
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                temp1[i][j] = (i == j ? 1.0 : 0.0) - k[i] * h[j];
            }
        }
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int kk = 0; kk < n; ++kk) {
                    temp2[i][j] += temp1[i][kk] * p[kk][j];
                }
            }
        }
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                p[i][j] = 0.0;
                for (int kk = 0; kk < n; ++kk) {
                    p[i][j] += temp2[i][kk] * temp1[j][kk];
                }
                p[i][j] += r * k[i] * k[j];
            }
        }
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j) {
                const double m = 0.5 * (p[i][j] + p[j][i]);
                p[i][j] = m;
                p[j][i] = m;
            }
    }
};

// 正弦类模式：扩展卡尔曼滤波。
struct HarmonicFilter {
    int n = 0;
    int asIndex = 0;
    int acIndex = 0;
    int fIndex = 0;
    double x[6] = {0.0};
    double p[6][6] = {};
    double sse = 0.0;
    bool valid = true;

    void init(int dim, int ai, int ci, int fi,const double* x0, const double* variance) {
        n = dim;
        asIndex = ai;
        acIndex = ci;
        fIndex = fi;
        for (int i = 0; i < n; ++i) {
            x[i] = x0[i];
            for (int j = 0; j < n; ++j) {
                p[i][j] = (i == j) ? variance[i] : 0.0;
            }
        }
    }

    // h 为观测雅可比行，predicted 为当前状态的模型输出 s(x)，r 为观测噪声方差
    void update(const double* h, double predicted, double r, double observation) {
        if (!valid) return;

        double ph[6];
        double s = r;
        for (int i = 0; i < n; ++i) {
            double v = 0.0;
            for (int j = 0; j < n; ++j) v += p[i][j] * h[j];
            ph[i] = v;
            s += h[i] * v;
        }
        if (!std::isfinite(s) || s < 0.0) {
            valid = false;
            return;
        }
        if (s == 0.0) {
            if (!std::isfinite(predicted) || !std::isfinite(observation)) valid = false;
            return;
        }

        const double resid = observation - predicted;
        sse += resid * resid;

        double k[6];
        for (int i = 0; i < n; ++i) k[i] = ph[i] / s;
        for (int i = 0; i < n; ++i) x[i] += k[i] * resid;

        double temp1[6][6] = {0.0}, temp2[6][6] = {0.0};
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                temp1[i][j] = (i == j ? 1.0 : 0.0) - k[i] * h[j];
            }
        }
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                double v = 0.0;
                for (int kk = 0; kk < n; ++kk) {
                    v += temp1[i][kk] * p[kk][j];
                }
                temp2[i][j] = v;
            }
        }
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                double v = 0.0;
                for (int kk = 0; kk < n; ++kk) {
                    v += temp2[i][kk] * temp1[j][kk];
                }
                p[i][j] = v + r * k[i] * k[j];
            }
        }
    }
};
}

class SignalIdentifier::Impl {
public:
    Impl(SignalMode mode, double noise_stddev) : mode(mode) {
        measVar = noise_stddev * noise_stddev;

        switch (mode) {
            case SignalMode::Constant:        polyDim = 1; break;
            case SignalMode::Linear:          polyDim = 2; break;
            case SignalMode::Quadratic:       polyDim = 3; break;
            case SignalMode::Sine:            polyDim = 0; break;
            case SignalMode::QuadraticSine:   polyDim = 0; break;
        }
    }

    SignalMode mode;
    double noise_stddev;
    void update(double timestamp, double observation) {
        if (first) {
            t0 = timestamp;
            first = false;
            if (polyDim == 0) initBank(observation);
            else kf.init(polyDim, 1e12);
        }
        double tau = timestamp - t0;

        if (polyDim == 0) {
            updateBank(tau, observation);
            return;
        }

        double h[3] = {1.0, 0.0, 0.0};
        if (polyDim >= 2) h[1] = tau;
        if (polyDim >= 3) h[2] = 0.5 * tau * tau;
        kf.update(h, measVar, observation);
    }

    ParameterEstimate estimate() const {
        ParameterEstimate est;
        if (polyDim > 0) {
            for (int i = 0; i < polyDim; i++) {
                est.parameters[i] = kf.x[i];
            }
            return est;
        }

        int best = -1;
        double bestSse = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < bank.size(); ++i) {
            const HarmonicFilter& f = bank[i];
            if (!f.valid) continue;
            if (f.sse < bestSse) {
                bestSse = f.sse;
                best = static_cast<int>(i);
            }
        }

        const HarmonicFilter& f = bank[static_cast<std::size_t>(best)];
        const double as = f.x[f.asIndex];
        const double ac = f.x[f.acIndex];
        double amplitude = std::hypot(as, ac);
        double phase = std::atan2(ac, as);
        phase = remainder(phase, 2.0 * kPi);

        if (mode == SignalMode::Sine) {
            est.parameters[0] = f.x[0];                  // b
            est.parameters[1] = amplitude;               // A
            est.parameters[2] = f.x[f.fIndex];           // f
            est.parameters[3] = phase;                   // phi
        } else {
            est.parameters[0] = f.x[0];                  // c
            est.parameters[1] = f.x[1];                  // v
            est.parameters[2] = f.x[2];                  // a
            est.parameters[3] = amplitude;               // A
            est.parameters[4] = f.x[f.fIndex];           // f
            est.parameters[5] = phase;                   // phi
        }
        return est;
    }

private:
    static constexpr double kFreqMin = 0.2;
    static constexpr double kFreqMax = 2.0;
    static constexpr int kFreqSteps = 180;               // 频率网格间隔 0.01 Hz

    void initBank(double firstObservation) {
        // 频率先验标准差取 0.03 Hz（约 3 个网格步长），各滤波器可在局部连续收敛
        const double fVar = 0.03 * 0.03;

        for (int i = 0; i <= kFreqSteps; ++i) {
            const double f0 = kFreqMin + (kFreqMax - kFreqMin) * i / kFreqSteps;
            HarmonicFilter flt;
            if (mode == SignalMode::Sine) {
                const double x0[4] = {firstObservation, 1.0, 0.0, f0};
                const double var[4] = {100.0, 4.0, 4.0, fVar};
                flt.init(4, 1, 2, 3, x0, var);
            } else {
                const double x0[6] = {firstObservation, 0.0, 0.0, 1.0, 0.0, f0};
                const double var[6] = {100.0, 4.0, 0.25, 4.0, 4.0, fVar};
                flt.init(6, 3, 4, 5, x0, var);
            }
            bank.push_back(flt);
        }
    }

    void updateBank(double tau, double observation) {
        for (HarmonicFilter& f : bank) {
            if (!f.valid) continue;
            const double as = f.x[f.asIndex];
            const double ac = f.x[f.acIndex];
            const double w = 2.0 * kPi * f.x[f.fIndex] * tau;
            const double sw = std::sin(w);
            const double cw = std::cos(w);

            double h[6] = {0.0};
            h[0] = 1.0;
            double predicted = f.x[0];
            if (mode == SignalMode::QuadraticSine) {
                h[1] = tau;
                h[2] = 0.5 * tau * tau;
                predicted += f.x[1] * tau + f.x[2] * h[2];
            }
            h[f.asIndex] = sw;
            h[f.acIndex] = cw;
            predicted += as * sw + ac * cw;
            h[f.fIndex] = 2.0 * kPi * tau * (as * cw - ac * sw);

            f.update(h, predicted, measVar, observation);
        }
        lastObservation = observation;
    }

    bool first = true;
    int polyDim = 0;
    double t0 = 0.0;
    double measVar = 0.0;
    double lastObservation = 0.0;
    StaticKalmanFilter kf;
    std::vector<HarmonicFilter> bank;
};

SignalIdentifier::SignalIdentifier(SignalMode mode, double noise_stddev)
    : impl_(std::make_unique<Impl>(mode, noise_stddev)) {}

SignalIdentifier::~SignalIdentifier() = default;

void SignalIdentifier::update(double timestamp, double observation) {
    impl_->update(timestamp, observation);
}

ParameterEstimate SignalIdentifier::estimate() const {
    return impl_->estimate();
}