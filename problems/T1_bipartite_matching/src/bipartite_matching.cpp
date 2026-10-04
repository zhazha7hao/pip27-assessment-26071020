#include "bipartite_matching.hpp"

#include <memory>
#include <vector>

// 这是可编译的起始模板。请在此处实现带权二分图最大权匹配算法。
struct BipartiteMatcher::Impl {
    Impl(std::uint32_t left_count, std::uint32_t right_count, const std::vector<Edge>& edges)
        : left_count(left_count), right_count(right_count), edges(edges) {}

    std::uint32_t left_count;
    std::uint32_t right_count;
    std::vector<Edge> edges;
};

BipartiteMatcher::BipartiteMatcher(std::uint32_t left_count, std::uint32_t right_count,
                                   const std::vector<Edge>& edges)
    : impl_(std::make_unique<Impl>(left_count, right_count, edges)) {}

BipartiteMatcher::~BipartiteMatcher() = default;

std::vector<std::int32_t> BipartiteMatcher::maximumWeightMatching() const {
    // 请实现总权重最大的非空合法匹配；无边时返回全 -1 表示无解。
    // 当前占位实现只能处理无边输入，任何有边的测试都会失败。
    return std::vector<std::int32_t>(impl_->left_count, -1);
}
