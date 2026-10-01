#pragma once

// T0_ab 的官方接口副本。
// 测评机使用它编译测评程序，确保测试调用的接口不由考生提交控制。
class Solver {
public:
    // 保存待相加的两个整数。
    Solver(int a, int b);

    // 返回构造函数接收的两个整数之和。
    [[nodiscard]] int sum() const;

private:
    int a_;
    int b_;
};
