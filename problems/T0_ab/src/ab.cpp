#include "ab.hpp"

// 构造函数保存输入；测评机随后通过 sum() 获取计算结果。
Solver::Solver(int a, int b): a_(a), b_(b) {}

int Solver::sum() const {
    return a_ + b_;
}
