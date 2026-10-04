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
    // TODO: 返回总权重最大的合法匹配。
    // 当前占位实现只返回空匹配，不能通过包含正权边的测试。
    return std::vector<std::int32_t>(impl_->left_count, -1);
}
