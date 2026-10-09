#include "signal_identifier.hpp"

#include <stdexcept>
#include <cmath>

class StaticKalmanFilter {
    private:
        bool ready=false;
        int n;
        double x[6]={0.0};
        double p[6][6]={0.0};
    public:
        void init(int n,double prior){
            this->n=n;
            for(int i=0;i<n;i++){
                p[i][i]=prior;
            }
            ready=true;
        }   
        void update(const double* h, const double r, const double observation){
            if(!ready){
                throw std::runtime_error("StaticKalmanFilter not initialized");
            }
            double ph[6];
            double s=r;
            for (int i = 0; i < n; ++i) {
                double cnt = 0.0;
                for (int j = 0; j < n; ++j) cnt += p[i][j] * h[j];
                ph[i] = cnt;
                s += h[i] * cnt;
            }
            if (s==0) return;
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
            double temp1[6][6]={0.0},temp2[6][6]={0.0};
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    temp1[i][j] = (i == j ? 1.0 : 0.0) - k[i] * h[j];
                }
            }
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    for (int k = 0; k < n; ++k) {
                        temp2[i][j] += temp1[i][k] * p[k][j];
                    }
                }
            }
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    p[i][j] = 0.0;
                    for (int kk = 0; kk < n; ++kk) {
                        p[i][j] += temp2[i][kk] * temp1[j][kk];
                    }
                    p[i][j] += r*k[i]*k[j];
                }
            }
            for (int i = 0; i < n; ++i)                     // 对称化,消除小数计算误差
            for (int j = i + 1; j < n; ++j) {
                const double m = 0.5 * (p[i][j] + p[j][i]);
                p[i][j] = m;
                p[j][i] = m;
            }
        }
        double state(int i) const { return x[i]; }
};
class SignalIdentifier::Impl {
public:
    Impl(SignalMode mode, double noise_stddev) : mode(mode){
        measVar = noise_stddev * noise_stddev;

        switch (mode) {
            case SignalMode::Constant:        polyDim = 1; break;
            case SignalMode::Linear:          polyDim = 2; break;
            case SignalMode::Quadratic:       polyDim = 3; break;
            case SignalMode::Sine:            polyDim = 0; break;
            case SignalMode::QuadraticSine:   polyDim = 0; break;
        }
        kf.init(polyDim, 1e12);
    }
    SignalMode mode;
    double noise_stddev;
    void update(double timestamp, double observation) {
        if(first){
            t0=timestamp;
            first=false;
        }
        double tau=timestamp-t0;
        double h[3] = {1.0, 0.0, 0.0};
        if (polyDim >= 2) h[1] = tau;
        if (polyDim >= 3) h[2] = 0.5 * tau * tau;
        kf.update(h, measVar, observation);
    }
    ParameterEstimate estimate() const {
        ParameterEstimate est;
        for(int i=0;i<polyDim ;i++){
            est.parameters[i]=kf.state(i);
        }
        return est;
    }
    private:
        bool first=true;
        int polyDim;
        double t0;
        double measVar;
        StaticKalmanFilter kf;
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
