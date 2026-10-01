#pragma once

// T0_ab 的固定测评接口。
class Solver {
public:
    Solver(int a, int b);

    // 返回构造对象时传入的两个整数之和。
    [[nodiscard]] int sum() const;

private:
    int a_;
    int b_;
};