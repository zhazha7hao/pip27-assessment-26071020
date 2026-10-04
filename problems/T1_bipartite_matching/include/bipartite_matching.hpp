#pragma once

#include <cstdint>
#include <memory>
#include <vector>

// T1 的固定测评接口。左右两侧顶点编号均从 0 开始。
struct Edge {
    std::uint32_t left;
    std::uint32_t right;
    std::int64_t weight;
};

class BipartiteMatcher {
public:
    BipartiteMatcher(std::uint32_t left_count, std::uint32_t right_count,
                     const std::vector<Edge>& edges);
    ~BipartiteMatcher();

    // 返回左侧每个顶点匹配到的右侧顶点编号；未匹配时返回 -1。
    [[nodiscard]] std::vector<std::int32_t> maximumWeightMatching() const;

private:
    // 把算法细节留在 src/，保持公开接口足够小。
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
