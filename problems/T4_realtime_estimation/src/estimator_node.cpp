#include "t4_realtime_estimation/msg/signal_packet.hpp"

#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <rclcpp/rclcpp.hpp>

#include <chrono>
#include <cstdint>
#include <deque>
#include <vector>

namespace signal_estimator {

using Packet = t4_realtime_estimation::msg::SignalPacket;

constexpr std::int64_t kBasicMode = 0;
constexpr std::int64_t kExtra1Mode = 1;
constexpr std::int64_t kExtra2Mode = 2;
constexpr std::int64_t kExtra3Mode = 3;

constexpr double kPredictionStepSeconds = 0.005;
constexpr std::int64_t kPredictionStepNs = 5'000'000;

constexpr char kNodeName[] = "signal_estimator";
constexpr char kModeParameter[] = "assessment_mode";
constexpr char kObservationTopic[] = "/signal/observation";
constexpr char kEstimateTopic[] = "/signal/estimate";
constexpr char kPredictionTopic[] = "/signal/prediction";

// 每 tick 最多补发的网格点数；正常运行时每 tick 最多一个。
constexpr int kMaxCatchUpPerTick = 128;

// 一帧输入观测
struct Sample {
  std::int64_t stamp_ns = 0;
  double value = 0.0;
};

// 各模式的滤波 / 外推参数。
//   jerk_psd   : 加加速度白噪声功率谱密度（过程噪声强度）
//   vel_decay  : 外推时速度项的 OU 阻尼系数
//   acc_decay  : 外推时加速度项的指数阻尼系数
struct ModeTuning {
  std::size_t point_count = 1;
  double jerk_psd = 1000.0;
  double vel_decay = 1e9;
  double acc_decay = 1e9;
};
//参数
ModeTuning tuning_for_mode(std::int64_t mode) {
  switch (mode) {
    case kExtra1Mode:
      return {21, 100.0, 12.0, 6.0};
    case kExtra2Mode:
      return {41, 30.0, 4.0, 1e9};
    case kExtra3Mode:
      return {101, 100.0, 4.0, 1.5};
    default:
      return {1, 1000.0, 1e9, 1e9};
  }
}

struct Vec3 {
  double v[3] = {0.0, 0.0, 0.0};
};

struct Mat3 {
  double m[3][3] = {{0.0}};
};

inline Vec3 mat_vec_mul(const Mat3& A, const Vec3& x) {
  Vec3 y;
  for (int i = 0; i < 3; ++i) {
    y.v[i] = A.m[i][0] * x.v[0] + A.m[i][1] * x.v[1] + A.m[i][2] * x.v[2];
  }
  return y;
}

// 卡尔曼滤波
class KalmanFilter {
 public:
  void init(double p0, std::int64_t stamp_ns, double jerk_psd) {
    x_ = Vec3{p0, 0.0, 0.0};
    P_ = Mat3{};
    P_.m[0][0] = P_.m[1][1] = P_.m[2][2] = 1.0;
    R_ = 0.1;
    q_ = jerk_psd;
    last_stamp_ns_ = stamp_ns;
  }

  // 时间推进到 stamp_ns（含协方差传播）
  void predict(std::int64_t stamp_ns) {
    const double dt = static_cast<double>(stamp_ns - last_stamp_ns_) * 1e-9;
    if (dt <= 0.0) return;

    Mat3 F;
    F.m[0][0] = 1.0; F.m[0][1] = dt;  F.m[0][2] = 0.5 * dt * dt;
    F.m[1][0] = 0.0; F.m[1][1] = 1.0; F.m[1][2] = dt;
    F.m[2][0] = 0.0; F.m[2][1] = 0.0; F.m[2][2] = 1.0;

    // 连续时间 CA 模型的离散过程噪声协方差
    const double dt2 = dt * dt;
    const double dt3 = dt2 * dt;
    const double dt4 = dt3 * dt;
    const double dt5 = dt4 * dt;
    Mat3 Q;
    Q.m[0][0] = q_ * dt5 / 20.0; Q.m[0][1] = q_ * dt4 / 8.0; Q.m[0][2] = q_ * dt3 / 6.0;
    Q.m[1][0] = Q.m[0][1];       Q.m[1][1] = q_ * dt3 / 3.0; Q.m[1][2] = q_ * dt2 / 2.0;
    Q.m[2][0] = Q.m[0][2];       Q.m[2][1] = Q.m[1][2];      Q.m[2][2] = q_ * dt;

    // P = F P F^T + Q
    Mat3 FP;
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        double s = 0.0;
        for (int k = 0; k < 3; ++k) s += F.m[i][k] * P_.m[k][j];
        FP.m[i][j] = s;
      }
    }
    Mat3 Pn;
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        double s = 0.0;
        for (int k = 0; k < 3; ++k) s += FP.m[i][k] * F.m[j][k];  // F^T
        Pn.m[i][j] = s + Q.m[i][j];
      }
    }
    P_ = Pn;
    x_ = mat_vec_mul(F, x_);
    last_stamp_ns_ = stamp_ns;
  }

  // 用一帧观测更新状态
  void update(double z) {
    const double innovation = z - x_.v[0];
    const double p00 = P_.m[0][0];
    const double S = P_.m[0][0] + R_;

    double K[3];
    K[0] = p00 / S;
    K[1] = P_.m[1][0] / S;
    K[2] = P_.m[2][0] / S;

    x_.v[0] += K[0] * innovation;
    x_.v[1] += K[1] * innovation;
    x_.v[2] += K[2] * innovation;

    const double A[3][3] = {{1.0 - K[0], 0.0, 0.0},
                            {-K[1],      1.0, 0.0},
                            {-K[2],      0.0, 1.0}};
    // AP = A · P
    double AP[3][3];
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        AP[i][j] = A[i][0] * P_.m[0][j] + A[i][1] * P_.m[1][j] + A[i][2] * P_.m[2][j];
      }
    }
    // P = AP · A^T + R·K·K^T
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        const double s = AP[i][0] * A[j][0] + AP[i][1] * A[j][1] + AP[i][2] * A[j][2];
        P_.m[i][j] = s + R_ * K[i] * K[j];
      }
    }

    // 观测噪声方差的自适应估计：新息平方的指数滑动平均
    constexpr double kAlpha = 0.95;
    R_ = kAlpha * R_ + (1.0 - kAlpha) * (innovation * innovation - p00);
    if (R_ < 1e-6) R_ = 1e-6;
    if (R_ > 1e4) R_ = 1e4;
  }

  Vec3 state_at(std::int64_t stamp_ns) const {
    Vec3 s = x_;
    const double dt = static_cast<double>(stamp_ns - last_stamp_ns_) * 1e-9;
    if (dt > 0.0) {
      const double p = s.v[0] + s.v[1] * dt + 0.5 * s.v[2] * dt * dt;
      const double v = s.v[1] + s.v[2] * dt;
      s.v[0] = p;
      s.v[1] = v;
    }
    return s;
  }

  // 从 state_at() 给出的状态外推 tau 秒
  static double extrapolate(const Vec3& s, double tau, double vel_decay, double acc_decay) {
    const double vel_term = s.v[1] * (1.0 - std::exp(-vel_decay * tau)) / vel_decay;
    const double acc_term = 0.5 * s.v[2] * tau * tau * std::exp(-acc_decay * tau);
    return s.v[0] + vel_term + acc_term;
  }

 private:
  Vec3 x_;
  Mat3 P_;
  double R_ = 1.0;
  double q_ = 1000.0;
  std::int64_t last_stamp_ns_ = 0;
};

//ROS 2 节点
class EstimatorNode final : public rclcpp::Node {
 public:
  EstimatorNode() : Node(kNodeName) {
    mode_ = declare_parameter<std::int64_t>(kModeParameter, -1);
    tuning_ = tuning_for_mode(mode_);

    const auto qos = rclcpp::QoS(rclcpp::KeepLast(256)).reliable().durability_volatile();

    observation_subscription_ = create_subscription<Packet>(
        kObservationTopic, qos,
        [this](Packet::ConstSharedPtr msg, const rclcpp::MessageInfo& info) {
          onObservation(msg, info);
        });
    estimate_publisher_ = create_publisher<Packet>(kEstimateTopic, qos);
    prediction_publisher_ = create_publisher<Packet>(kPredictionTopic, qos);

    prediction_timer_ = create_wall_timer(
        std::chrono::milliseconds(1), [this]() { onPredictionTimer(); });

    parameter_callback_ = add_on_set_parameters_callback(
        [this](const std::vector<rclcpp::Parameter>& params) {
          return onSetParameters(params);
        });
  }

 private:
  void publish_packet(rclcpp::Publisher<Packet>::SharedPtr& pub,
                      std::int64_t stamp_ns,
                      const std::vector<double>& values) {
    Packet msg;
    msg.header.stamp = rclcpp::Time(stamp_ns);
    msg.values = values;
    pub->publish(msg);
  }
  void onObservation(const Packet::ConstSharedPtr& message,const rclcpp::MessageInfo& message_info) {
    const std::int64_t stamp_ns = rclcpp::Time(message->header.stamp).nanoseconds();
    const double value = message->values.empty() ? 0.0 : message->values[0];

    if (!first_obs_received_) {
      first_obs_received_ = true;
      anchor_virtual_ns_ = stamp_ns; // 首帧观测时间戳
      anchor_real_time_ = resolve_anchor_real_time(message_info); // 首帧观测本机时间
      next_virtual_stamp_ns_ = stamp_ns;
      kf_.init(value, stamp_ns, tuning_.jerk_psd);

      if (mode_ == kBasicMode) {
        publish_packet(estimate_publisher_, stamp_ns, {value});
      } else {
        obs_buffer_.push_back({stamp_ns, value});
      }
      return;
    }

    if (mode_ == kBasicMode) {
      // 基础模式：每帧观测立即输出当前时刻的滤波值
      kf_.predict(stamp_ns);
      kf_.update(value);
      publish_packet(estimate_publisher_, stamp_ns, {kf_.state_at(stamp_ns).v[0]});
    } else {
      obs_buffer_.push_back({stamp_ns, value});
    }
  }

  // 把首帧发布时间映射到本进程的单调时钟：
  //   anchor = now_steady - (now_wall - dds_source_timestamp)
  // 用回调到达时刻当零点会把首帧通信/回调延迟固定平移到后面所有预测包上。
  std::chrono::steady_clock::time_point resolve_anchor_real_time(
      const rclcpp::MessageInfo& message_info) const {
    const std::int64_t src_ns =
        static_cast<std::int64_t>(message_info.get_rmw_message_info().source_timestamp); // DDS 时间戳
    const std::int64_t wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count(); // 系统时钟时间戳
    const std::int64_t latency_ns = wall_ns - src_ns; // 发送到本进程的延迟

    return std::chrono::steady_clock::now() - std::chrono::nanoseconds(latency_ns); // 首帧观测本机时间
  }

  void onPredictionTimer() {
    if (!first_obs_received_ || mode_ == kBasicMode || mode_ < 0) return;

    const auto now = std::chrono::steady_clock::now();
    // 只发布已到点的网格，绝不提前；若某个 tick 被拖延，则在同一 tick 内补齐，
    // 避免定时器抖动导致必需起点被整段跳过。
    for (int i = 0; i < kMaxCatchUpPerTick; ++i) {
      const auto target = anchor_real_time_ + std::chrono::nanoseconds(next_virtual_stamp_ns_ - anchor_virtual_ns_);
      if (now < target) break;
      publish_prediction_at(next_virtual_stamp_ns_);
      next_virtual_stamp_ns_ += kPredictionStepNs;
    }
  }

  void publish_prediction_at(std::int64_t grid_ns) {
    // 只使用时间戳不晚于本起点的观测
    while (!obs_buffer_.empty() && obs_buffer_.front().stamp_ns <= grid_ns) {
      const Sample s = obs_buffer_.front();
      obs_buffer_.pop_front();
      kf_.predict(s.stamp_ns);
      kf_.update(s.value);
    }

    const Vec3 state = kf_.state_at(grid_ns);
    std::vector<double> values(tuning_.point_count);
    for (std::size_t i = 0; i < tuning_.point_count; i++) {
      const double tau = static_cast<double>(i) * kPredictionStepSeconds;
      values[i] = KalmanFilter::extrapolate(state, tau, tuning_.vel_decay, tuning_.acc_decay);
    }

    publish_packet(prediction_publisher_, grid_ns, values);
  }

  rcl_interfaces::msg::SetParametersResult onSetParameters(
      const std::vector<rclcpp::Parameter>& parameters) {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    for (const auto& param : parameters) {
      if (param.get_name() != kModeParameter) continue;
      const std::int64_t new_mode = param.as_int();
      if (new_mode < 0 || new_mode > 3) {
        result.successful = false;
        result.reason = "assessment_mode must be in [0, 3]";
        return result;
      }
      mode_ = new_mode;
      tuning_ = tuning_for_mode(mode_);
    }
    return result;
  }

  std::int64_t mode_ = -1;
  ModeTuning tuning_;
  KalmanFilter kf_;

  bool first_obs_received_ = false;
  std::int64_t anchor_virtual_ns_ = 0;
  std::chrono::steady_clock::time_point anchor_real_time_;
  std::int64_t next_virtual_stamp_ns_ = 0;
  std::deque<Sample> obs_buffer_;

  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_callback_;
  rclcpp::Subscription<Packet>::SharedPtr observation_subscription_;
  rclcpp::Publisher<Packet>::SharedPtr estimate_publisher_;
  rclcpp::Publisher<Packet>::SharedPtr prediction_publisher_;
  rclcpp::TimerBase::SharedPtr prediction_timer_;
};

}

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<signal_estimator::EstimatorNode>());
  rclcpp::shutdown();
  return 0;
}