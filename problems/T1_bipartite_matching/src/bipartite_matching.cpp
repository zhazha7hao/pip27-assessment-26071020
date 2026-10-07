#include "bipartite_matching.hpp"

#include <memory>
#include <vector>
#include <queue>

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
    const std::uint32_t L = impl_->left_count;
    const std::uint32_t R = impl_->right_count;
    const auto& edges = impl_->edges;

    // 默认初始化全 -1 表示未匹配
    std::vector<std::int32_t> match_left(L, -1);

    // 1. 无边图直接返回全 -1（无解）
    if (edges.empty()) {
        return match_left;
    }

    // 2. 扫描最大边权与最佳单边
    std::int64_t max_edge_weight = -1e9;
    std::size_t best_edge_idx = 0;

    for (std::size_t i = 0; i < edges.size(); ++i) {
        auto [u, v, weight] = edges[i];
        if (weight > max_edge_weight) {
            max_edge_weight = weight;
            best_edge_idx = i;
        }
    }

    // 3. 当所有边权均 <= 0 时，最优非空匹配必为单条最大权边
    if (max_edge_weight <= 0) {
        auto [u, v, weight] = edges[best_edge_idx];
        match_left[u] = v;
        return match_left;
    }

    // 4. 存在正权边时，构建费用流模型
    struct FlowEdge {
        int to;
        int rev;
        int cap;             // 残量容量 (0 或 1)
        std::int64_t cost;   // 费用 (-weight)
    };

    const int S = 0;
    const int T = L + R + 1;
    const int num_nodes = L + 1 + R + 1;
    const std::int64_t INF = 1e18;

    std::vector<std::vector<FlowEdge>> adj(num_nodes);

    auto add_graph_edge = [&](int u_node, int v_node, std::int64_t cost) {
        adj[u_node].push_back({v_node, adj[v_node].size(), 1, cost});
        adj[v_node].push_back({u_node, adj[u_node].size() - 1, 0, -cost});
    };

    // 建图: S -> 左部点, 右部点 -> T, 左部点 -> 右部点
    for (std::uint32_t u = 0; u < L; u++) {
        add_graph_edge(S, u + 1, 0);
    }
    for (std::uint32_t v = 0; v < R; v++) {
        add_graph_edge(v + L + 1, T, 0);
    }
    for (const auto& edge : edges) {
        auto [u, v, weight] = edge;
        int u_node = u + 1;
        int v_node = L + v + 1;
        add_graph_edge(u_node, v_node, -weight);
    }

    while (true) {
        std::vector<std::int64_t> dist(num_nodes, INF);
        std::vector<int> parent_node(num_nodes, -1), parent_edge(num_nodes, -1);
        std::vector<bool> in_queue(num_nodes, false);
        std::queue<int> q;

        dist[S] = 0;
        q.push(S);
        in_queue[S] = true;

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            in_queue[u] = false;

            for (std::size_t i = 0; i < adj[u].size(); ++i) {
                const auto& edge = adj[u][i];
                if (edge.cap > 0 && dist[u] + edge.cost < dist[edge.to]) {
                    dist[edge.to] = dist[u] + edge.cost;
                    parent_node[edge.to] = u;
                    parent_edge[edge.to] = i;
                    if (!in_queue[edge.to]) {
                        q.push(edge.to);
                        in_queue[edge.to] = true;
                    }
                }
            }
        }

        if (dist[T] == INF) break;

        if (-dist[T] <= 0) break; // 收益非正，停止推流

        // 沿增广路推流
        int curr = T;
        while (curr != S) {
            int prev = parent_node[curr];
            int e_idx = parent_edge[curr];
            adj[prev][e_idx].cap -= 1;
            int rev_idx = adj[prev][e_idx].rev;
            adj[curr][rev_idx].cap += 1;
            curr = prev;
        }
    }

    for (std::uint32_t u = 0; u < L; ++u) {
        int u_node = u + 1;
        for (const auto& edge : adj[u_node]) {
            if (edge.to >= static_cast<int>(L) + 1 && edge.to <= static_cast<int>(L + R) && edge.cap == 0) {
                match_left[u] = edge.to - L - 1;
                break;
            }
        }
    }

    return match_left;
}
