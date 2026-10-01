#include "ab.hpp"

#include <iostream>

int main() {
    // 演示创建对象及调用成员函数，并检查两组结果。
    if (Solver(2, 3).sum() != 5 || Solver(4, 3).sum() != 7) {
        std::cerr << "Local check failed\n";
        return 1;
    }

    std::cout << "Local check passed\n";
    return 0;
}