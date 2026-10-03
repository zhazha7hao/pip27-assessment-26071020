#pragma once

#include <cstddef>
#include <memory>
#include <vector>

// T2 的固定测评接口。下标从 0 开始，查询排名 k 从 1 开始。
class DynamicKth {
public:
    explicit DynamicKth(const std::vector<int>& initial);
    ~DynamicKth();

    // 在 index 位置之前插入；index 等于当前长度时追加到末尾。
    void insert(std::size_t index, int value);

    // 删除或修改当前 index 位置的元素。
    void erase(std::size_t index);
    void update(std::size_t index, int value);

    // 返回闭区间 [left, right] 中第 k 大的值。
    [[nodiscard]] int kthLargest(
        std::size_t left, std::size_t right, std::size_t k) const;

private:
    // 实现细节放在 src/ 中
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
