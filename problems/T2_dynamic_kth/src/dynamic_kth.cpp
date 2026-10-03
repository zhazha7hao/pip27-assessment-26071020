#include "dynamic_kth.hpp"

#include <stdexcept>

// 在此定义存储结构并实现算法；初始空结构仅保证模板可以编译。
struct DynamicKth::Impl {};

DynamicKth::DynamicKth(const std::vector<int>&)
    : impl_(std::make_unique<Impl>()) {
    // TODO: 用 `initial` 构造序列，允许初始序列为空

}

DynamicKth::~DynamicKth() = default;

void DynamicKth::insert(std::size_t, int) {
    // TODO: 在当前 `index` 位置之前插入；`index == 当前长度` 表示末尾追加

}

void DynamicKth::erase(std::size_t) {
    // TODO: 删除当前 `index` 位置的元素

}

void DynamicKth::update(std::size_t, int) {
    // TODO: 将当前 `index` 位置的元素修改为 `value`

}

int DynamicKth::kthLargest(std::size_t, std::size_t, std::size_t) const {
    // TODO: 返回当前闭区间 `[left, right]` 中第 `k` 大的值，`k` 从 **1** 开始
    throw std::logic_error("DynamicKth::kthLargest 尚未实现");
}
