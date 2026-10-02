#include "dynamic_kth.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        // 同时检查初始序列、区间查询和插入后下标的变化。
        DynamicKth sequence({5, 1, 3});
        if (sequence.kthLargest(0, 2, 2) != 3) {
            std::cerr << "Local check failed after construction\n";
            return 1;
        }

        sequence.insert(1, 4);  // [5, 4, 1, 3]
        if (sequence.kthLargest(1, 3, 2) != 3) {
            std::cerr << "Local check failed after insertion\n";
            return 1;
        }

        sequence.erase(0);     // [4, 1, 3]
        sequence.update(1, 6);  // [4, 6, 3]
        if (sequence.kthLargest(0, 2, 1) != 6 ||
            sequence.kthLargest(0, 2, 3) != 3) {
            std::cerr << "Local check failed after deletion or update\n";
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "Local check failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "Local check passed\n";
    return 0;
}
