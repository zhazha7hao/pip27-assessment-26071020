#include "dynamic_kth.hpp"

#include <stdexcept>

// 在此定义存储结构并实现算法；初始空结构仅保证模板可以编译。
struct DynamicKth::Impl {};

DynamicKth::DynamicKth(const std::vector<int>&)
    : impl_(std::make_unique<Impl>()) {}

DynamicKth::~DynamicKth() = default;

void DynamicKth::insert(std::size_t, int) {
    throw std::logic_error("DynamicKth::insert 尚未实现");
}

void DynamicKth::erase(std::size_t) {
    throw std::logic_error("DynamicKth::erase 尚未实现");
}

void DynamicKth::update(std::size_t, int) {
    throw std::logic_error("DynamicKth::update 尚未实现");
}

int DynamicKth::kthLargest(std::size_t, std::size_t, std::size_t) const {
    throw std::logic_error("DynamicKth::kthLargest 尚未实现");
}
