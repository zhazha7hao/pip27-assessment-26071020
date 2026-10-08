#include "bipartite_matching.hpp"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <random>
#include <chrono>
#include <algorithm>
#include <unordered_set>

// ========================================
// 暴力算法：状态压缩 DP（用于小规模验证）
// ========================================
std::int64_t brute_force_max_weight(int L, int R, const std::vector<Edge>& edges) {
    // 无边时返回 INT64_MIN 表示无解
    if (edges.empty()) {
        return INT64_MIN;
    }
    
    std::vector<std::vector<std::int64_t>> weight(L, std::vector<std::int64_t>(R, INT64_MIN));
    for (const auto& e : edges) {
        weight[e.left][e.right] = e.weight;
    }
    
    const std::int64_t NEG_INF = INT64_MIN / 2; // 避免溢出
    // dp[mask] 表示考虑前 i 个左顶点，右部使用状态为 mask 时的最大权重
    std::vector<std::int64_t> dp(1 << R, NEG_INF);
    dp[0] = 0;
    
    for (int u = 0; u < L; ++u) {
        std::vector<std::int64_t> new_dp = dp; // 不匹配 u 的情况
        for (int mask = 0; mask < (1 << R); ++mask) {
            if (dp[mask] == NEG_INF) continue;
            for (int v = 0; v < R; ++v) {
                if (!(mask & (1 << v)) && weight[u][v] != INT64_MIN) {
                    int new_mask = mask | (1 << v);
                    new_dp[new_mask] = std::max(new_dp[new_mask], dp[mask] + weight[u][v]);
                }
            }
        }
        dp = new_dp;
    }
    
    // 找最大值，必须非空（mask > 0）
    std::int64_t ans = NEG_INF;
    for (int mask = 1; mask < (1 << R); ++mask) {
        if (dp[mask] > ans) {
            ans = dp[mask];
        }
    }
    return ans;
}

// ========================================
// 验证匹配的合法性
// ========================================
bool validate_matching(const std::vector<std::int32_t>& result, 
                       const std::vector<Edge>& edges,
                       std::int64_t& total_weight) {
    int L = result.size();
    std::unordered_set<int> used_right;
    total_weight = 0;
    
    // 构建边集哈希表用于快速查找
    std::unordered_set<std::uint64_t> edge_set;
    for (const auto& e : edges) {
        edge_set.insert((static_cast<std::uint64_t>(e.left) << 32) | e.right);
    }
    
    for (int u = 0; u < L; ++u) {
        if (result[u] != -1) {
            int v = result[u];
            // 检查右顶点是否重复
            if (used_right.count(v)) {
                std::cout << "  错误：右顶点 " << v << " 被重复使用\n";
                return false;
            }
            used_right.insert(v);
            
            // 检查边是否存在
            std::uint64_t key = (static_cast<std::uint64_t>(u) << 32) | v;
            if (!edge_set.count(key)) {
                std::cout << "  错误：边 (" << u << ", " << v << ") 不存在\n";
                return false;
            }
            
            // 累加权重
            for (const auto& e : edges) {
                if (e.left == static_cast<std::uint32_t>(u) && e.right == static_cast<std::uint32_t>(v)) {
                    total_weight += e.weight;
                    break;
                }
            }
        }
    }
    
    return true;
}

// ========================================
// 运行测试并验证
// ========================================
void run_test(const std::string& test_name, 
              std::uint32_t L, std::uint32_t R, 
              const std::vector<Edge>& edges,
              bool check_against_brute_force = false) {
    std::cout << "========================================\n";
    std::cout << "[测试项]: " << test_name << "\n";
    std::cout << "L=" << L << ", R=" << R << ", E=" << edges.size() << "\n";
    
    BipartiteMatcher matcher(L, R, edges);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = matcher.maximumWeightMatching();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 验证返回数组长度
    if (result.size() != L) {
        std::cout << "状态: [FAIL] - 返回数组长度错误，期望 " << L << "，实际 " << result.size() << "\n";
        assert(false);
        return;
    }
    
    // 验证匹配合法性
    std::int64_t total_weight = 0;
    bool valid = validate_matching(result, edges, total_weight);
    
    if (!valid) {
        std::cout << "状态: [FAIL] - 匹配不合法\n";
        assert(false);
        return;
    }
    
    // 检查非空约束
    bool has_match = false;
    for (int v : result) {
        if (v != -1) {
            has_match = true;
            break;
        }
    }
    
    if (!edges.empty() && !has_match) {
        std::cout << "状态: [FAIL] - 有边但返回空匹配\n";
        assert(false);
        return;
    }
    
    if (edges.empty() && has_match) {
        std::cout << "状态: [FAIL] - 无边但返回非空匹配\n";
        assert(false);
        return;
    }
    
    int matched_count = std::count_if(result.begin(), result.end(), [](int x) { return x != -1; });
    std::cout << "选中的边数: " << matched_count << "\n";
    std::cout << "总匹配权重: " << total_weight << "\n";
    std::cout << "运行时间: " << duration << " ms\n";
    
    // 与暴力算法对比
    if (check_against_brute_force && L <= 8 && R <= 8 && !edges.empty()) {
        std::int64_t expected = brute_force_max_weight(L, R, edges);
        if (total_weight != expected) {
            std::cout << "状态: [FAIL] - 权重不匹配，期望 " << expected << "，实际 " << total_weight << "\n";
            assert(false);
            return;
        }
        std::cout << "暴力验证: 通过 (期望权重=" << expected << ")\n";
    }
    
    std::cout << "状态: [PASS]\n";
}

// ========================================
// 生成随机图
// ========================================
std::vector<Edge> generate_random_edges(std::uint32_t L, std::uint32_t R, std::size_t E, 
                                        std::int64_t min_weight, std::int64_t max_weight,
                                        std::mt19937& gen) {
    std::vector<Edge> edges;
    std::unordered_set<std::uint64_t> used;
    std::uniform_int_distribution<std::uint32_t> left_dist(0, L - 1);
    std::uniform_int_distribution<std::uint32_t> right_dist(0, R - 1);
    std::uniform_int_distribution<std::int64_t> weight_dist(min_weight, max_weight);
    
    while (edges.size() < E) {
        std::uint32_t u = left_dist(gen);
        std::uint32_t v = right_dist(gen);
        std::uint64_t key = (static_cast<std::uint64_t>(u) << 32) | v;
        if (!used.count(key)) {
            used.insert(key);
            edges.push_back({u, v, weight_dist(gen)});
        }
    }
    
    return edges;
}

// ========================================
// 主函数
// ========================================
int main() {
    std::cout << "========================================\n";
    std::cout << "T1 带权二分图最大权匹配 - 测试程序\n";
    std::cout << "========================================\n\n";
    
    // ========================================
    // 第一部分：基础测试（保留原有测试用例）
    // ========================================
    std::cout << "【第一部分：基础测试】\n\n";
    
    // 测试 1：空边图
    run_test("1. 空边图测试 (E=0)", 3, 3, {}, true);
    
    // 测试 2：全负权边
    run_test("2. 全负权边测试", 2, 2, {{0, 0, -10}, {1, 1, -2}}, true);
    
    // 测试 3：最大边权为零
    run_test("3. 最大边权为零测试", 2, 2, {{0, 0, -5}, {0, 1, 0}}, true);
    
    // 测试 4：标准正权图
    run_test("4. 标准正权图测试", 2, 2, {{0, 0, 10}, {0, 1, 5}, {1, 0, 8}, {1, 1, 12}}, true);
    
    // 测试 5：权重优先而非数量优先
    run_test("5. 权重优先而非数量优先测试", 2, 2, {{0, 0, 100}, {0, 1, 10}, {1, 0, 10}}, true);
    
    // 测试 6：大整数权值
    run_test("6. 64位大整数权值测试", 2, 2, {{0, 0, 1000000000000LL}, {1, 1, 1000000000000LL}}, true);
    
    // 测试 7：零顶点边界
    run_test("7. 零顶点边界测试", 0, 0, {}, true);
    
    // ========================================
    // 第二部分：随机小规模测试（与暴力算法对比）
    // ========================================
    std::cout << "\n【第二部分：随机小规模测试（与暴力算法对比）】\n\n";
    
    std::mt19937 gen(42); // 固定种子保证可重复性
    
    for (int test_id = 0; test_id < 20; ++test_id) {
        std::uniform_int_distribution<std::uint32_t> size_dist(1, 8);
        std::uint32_t L = size_dist(gen);
        std::uint32_t R = size_dist(gen);
        std::size_t max_E = L * R;
        std::uniform_int_distribution<std::size_t> e_dist(0, max_E);
        std::size_t E = e_dist(gen);
        
        std::uniform_int_distribution<std::int64_t> weight_dist(-100, 100);
        auto edges = generate_random_edges(L, R, E, -100, 100, gen);
        
        std::string name = "随机测试 " + std::to_string(test_id + 1);
        run_test(name, L, R, edges, true);
    }
    
    // ========================================
    // 第三部分：稠密图测试（拓展1和拓展2）
    // ========================================
    std::cout << "\n【第三部分：稠密图测试】\n\n";
    
    // 拓展1：L=200, R=200，稠密图
    {
        std::mt19937 gen1(123);
        auto edges = generate_random_edges(200, 200, 200 * 200, -1000, 1000, gen1);
        run_test("拓展1: L=200, R=200 稠密图", 200, 200, edges, false);
    }
    
    // 拓展2：L=500, R=500，稠密图
    {
        std::mt19937 gen2(456);
        auto edges = generate_random_edges(500, 500, 500 * 500, -1000, 1000, gen2);
        run_test("拓展2: L=500, R=500 稠密图", 500, 500, edges, false);
    }
    
    // ========================================
    // 第四部分：稀疏图测试（拓展3）
    // ========================================
    std::cout << "\n【第四部分：稀疏图测试】\n\n";
    
    // 拓展3：L=2000, R=2000, E=50000，稀疏图
    {
        std::mt19937 gen3(789);
        auto edges = generate_random_edges(2000, 2000, 50000, -1000, 1000, gen3);
        run_test("拓展3: L=2000, R=2000, E=50000 稀疏图", 2000, 2000, edges, false);
    }
    
    // ========================================
    // 第五部分：特殊边界测试
    // ========================================
    std::cout << "\n【第五部分：特殊边界测试】\n\n";
    
    // 测试：单边图
    run_test("单边图测试", 1, 1, {{0, 0, 42}}, true);
    
    // 测试：单边负权图
    run_test("单边负权图测试", 1, 1, {{0, 0, -42}}, true);
    
    // 测试：单边零权图
    run_test("单边零权图测试", 1, 1, {{0, 0, 0}}, true);
    
    // 测试：L=1, R=8
    run_test("L=1, R=8 测试", 1, 8, {{0, 0, 1}, {0, 1, 5}, {0, 2, 3}, {0, 3, 2}}, true);
    
    // 测试：L=8, R=1
    run_test("L=8, R=1 测试", 8, 1, {{0, 0, 1}, {1, 0, 5}, {2, 0, 3}, {3, 0, 2}}, true);
    
    // 测试：全零权边
    run_test("全零权边测试", 3, 3, {{0, 0, 0}, {1, 1, 0}, {2, 2, 0}}, true);
    
    // 测试：混合正负权
    run_test("混合正负权测试", 3, 3, {{0, 0, -5}, {0, 1, 10}, {1, 0, 8}, {1, 1, -3}, {2, 2, 7}}, true);
    
    std::cout << "\n========================================\n";
    std::cout << "恭喜！所有测试均通过！\n";
    std::cout << "========================================\n";
    
    return 0;
}
