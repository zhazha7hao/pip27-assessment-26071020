#include "dynamic_kth.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <random>
#include <unordered_set>
#include <chrono>
#include <iomanip>

// -----------------------------------------------------------------------------
// 1. 暴力朴素参照实现（小规模对拍专用）
// -----------------------------------------------------------------------------
class NaiveDynamicKth {
public:
    explicit NaiveDynamicKth(const std::vector<int>& initial) : data_(initial) {}

    void insert(std::size_t index, int value) {
        data_.insert(data_.begin() + index, value);
    }

    void erase(std::size_t index) {
        data_.erase(data_.begin() + index);
    }

    void update(std::size_t index, int value) {
        data_[index] = value;
    }

    int kthLargest(std::size_t left, std::size_t right, std::size_t k) const {
        std::vector<int> sub(data_.begin() + left, data_.begin() + right + 1);
        std::sort(sub.rbegin(), sub.rend());
        return sub[k - 1];
    }

    std::size_t size() const { return data_.size(); }
    const std::vector<int>& data() const { return data_; }

private:
    std::vector<int> data_;
};

// -----------------------------------------------------------------------------
// 2. 基础功能测试
// -----------------------------------------------------------------------------
void test_basic() {
    std::cout << "[Test 1] 基础功能测试..." << std::endl;

    std::vector<int> init = {10, 20, 30, 40, 50};
    DynamicKth dk(init);

    assert(dk.kthLargest(0, 4, 1) == 50);
    assert(dk.kthLargest(0, 4, 3) == 30);
    assert(dk.kthLargest(0, 4, 5) == 10);

    dk.insert(2, 25); // [10, 20, 25, 30, 40, 50]
    assert(dk.kthLargest(1, 3, 1) == 30);
    assert(dk.kthLargest(1, 3, 2) == 25);

    dk.erase(0); // [20, 25, 30, 40, 50]
    assert(dk.kthLargest(0, 4, 1) == 50);

    dk.update(0, 100); // [100, 25, 30, 40, 50]
    assert(dk.kthLargest(0, 2, 1) == 100);

    std::cout << "  ==> 基础功能测试通过！\n" << std::endl;
}

// -----------------------------------------------------------------------------
// 3. 小规模准确性对拍测试
// -----------------------------------------------------------------------------
void test_small_stress(int total_ops = 5000) {
    std::cout << "[Test 2] 运行正确性对拍测试 (N=2000, Q=" << total_ops << ")..." << std::endl;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);

    std::unordered_set<int> exist_vals;
    std::vector<int> init_vals;

    auto get_unique_val = [&]() {
        int v;
        do { v = val_dist(rng); } while (exist_vals.count(v));
        exist_vals.insert(v);
        return v;
    };

    for (int i = 0; i < 2000; ++i) {
        init_vals.push_back(get_unique_val());
    }

    DynamicKth dk(init_vals);
    NaiveDynamicKth naive(init_vals);

    for (int op = 0; op < total_ops; ++op) {
        int type = rng() % 7;
        std::size_t cur_len = naive.size();

        if ((type == 0 || cur_len < 5) && cur_len < 5000) {
            std::size_t idx = rng() % (cur_len + 1);
            int val = get_unique_val();
            dk.insert(idx, val);
            naive.insert(idx, val);
        } else if (type == 1 && cur_len > 1) {
            std::size_t idx = rng() % cur_len;
            exist_vals.erase(naive.data()[idx]);
            dk.erase(idx);
            naive.erase(idx);
        } else if (type == 2 && cur_len > 0) {
            std::size_t idx = rng() % cur_len;
            exist_vals.erase(naive.data()[idx]);
            int val = get_unique_val();
            dk.update(idx, val);
            naive.update(idx, val);
        } else if (cur_len > 0) {
            std::size_t l = rng() % cur_len;
            std::size_t r = rng() % cur_len;
            if (l > r) std::swap(l, r);
            std::size_t range_len = r - l + 1;
            std::size_t k = (rng() % range_len) + 1;

            int ans_dk = dk.kthLargest(l, r, k);
            int ans_naive = naive.kthLargest(l, r, k);

            if (ans_dk != ans_naive) {
                std::cerr << "\n[Error] 对拍失败！Op " << op << ":区间[" << l << "," << r << "], k=" << k
                          << " | DynamicKth=" << ans_dk << ", Naive=" << ans_naive << "\n";
                assert(false);
            }
        }
    }
    std::cout << "  ==> 对拍测试通过，算法逻辑完全正确！\n" << std::endl;
}

// -----------------------------------------------------------------------------
// 4. 基础边界测试
// -----------------------------------------------------------------------------
void test_basic_edge_cases() {
    std::cout << "[Test 3] 基础边界测试..." << std::endl;

    // 空初始序列
    std::vector<int> empty_init;
    DynamicKth dk_empty(empty_init);
    dk_empty.insert(0, 42);
    assert(dk_empty.kthLargest(0, 0, 1) == 42);
    dk_empty.erase(0);
    dk_empty.insert(0, 100);
    dk_empty.insert(1, 200);
    assert(dk_empty.kthLargest(0, 1, 1) == 200);
    assert(dk_empty.kthLargest(0, 1, 2) == 100);

    // 初始长度1
    std::vector<int> single_init = {50};
    DynamicKth dk_single(single_init);
    assert(dk_single.kthLargest(0, 0, 1) == 50);
    dk_single.update(0, 75);
    assert(dk_single.kthLargest(0, 0, 1) == 75);
    dk_single.insert(0, 25);
    dk_single.insert(2, 100);
    assert(dk_single.kthLargest(0, 2, 1) == 100);
    assert(dk_single.kthLargest(0, 2, 2) == 75);
    assert(dk_single.kthLargest(0, 2, 3) == 25);

    // 首尾操作
    std::vector<int> init = {10, 20, 30, 40, 50};
    DynamicKth dk(init);
    dk.insert(0, 5);   // 头部插入
    dk.insert(6, 60);  // 尾部插入
    assert(dk.kthLargest(0, 6, 1) == 60);
    assert(dk.kthLargest(0, 6, 7) == 5);
    dk.erase(0);  // 头部删除
    dk.erase(5);  // 尾部删除
    assert(dk.kthLargest(0, 4, 1) == 50);
    assert(dk.kthLargest(0, 4, 5) == 10);

    std::cout << "  ==> 基础边界测试通过！\n" << std::endl;
}

// -----------------------------------------------------------------------------
// 5. 数据生成函数
// -----------------------------------------------------------------------------
std::vector<int> generate_initial_data(std::size_t N, std::mt19937& rng) {
    std::vector<int> data(N);
    for (std::size_t i = 0; i < N; ++i) {
        data[i] = static_cast<int>(i * 2 - 1000000000LL);
    }
    std::shuffle(data.begin(), data.end(), rng);
    return data;
}

// 生成聚簇双峰值数据
std::vector<int> generate_clustered_bimodal_data(std::size_t N, std::mt19937& rng) {
    std::vector<int> data(N);
    for (std::size_t i = 0; i < N; ++i) {
        if (i % 2 == 0) {
            data[i] = -500000000 + static_cast<int>(rng() % 100000000);
        } else {
            data[i] = 400000000 + static_cast<int>(rng() % 100000000);
        }
    }
    std::shuffle(data.begin(), data.end(), rng);
    return data;
}

// 生成窄初始值域数据
std::vector<int> generate_narrow_range_data(std::size_t N, std::mt19937& rng) {
    std::vector<int> data(N);
    for (std::size_t i = 0; i < N; ++i) {
        data[i] = static_cast<int>(rng() % 1000) - 500;  // 窄范围 [-500, 500]
    }
    std::shuffle(data.begin(), data.end(), rng);
    return data;
}

// -----------------------------------------------------------------------------
// 6. 拓展1测试 (N=100000, Q=50000)
// -----------------------------------------------------------------------------

// 测试 6.1: 查询主导负载
double test_ext1_query_dominant() {
    constexpr std::size_t N = 100000;
    constexpr int Q = 50000;

    std::cout << "\n[Ext1-Test 1] 查询主导负载 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(1337);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << std::fixed << std::setprecision(2) << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        std::size_t l = rng() % cur_len;
        std::size_t r = rng() % cur_len;
        if (l > r) std::swap(l, r);
        std::size_t k = (rng() % (r - l + 1)) + 1;
        int res = dk.kthLargest(l, r, k);
        (void)res;
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 查询主导测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 6.2: 变更主导负载
double test_ext1_update_dominant() {
    constexpr std::size_t N = 100000;
    constexpr int Q = 50000;

    std::cout << "\n[Ext1-Test 2] 变更主导负载 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(2024);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 10;
        if (type < 5 && cur_len > 0) {
            // 50% 修改
            std::size_t idx = rng() % cur_len;
            dk.update(idx, val_dist(rng));
        } else if (type < 7 && cur_len < N + 1000) {
            // 20% 插入
            std::size_t idx = rng() % (cur_len + 1);
            dk.insert(idx, val_dist(rng));
            cur_len++;
        } else if (cur_len > 1) {
            // 30% 删除
            std::size_t idx = rng() % cur_len;
            dk.erase(idx);
            cur_len--;
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次变更操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 变更主导测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 6.3: 查询占比 50% 负载
double test_ext1_mixed_50() {
    constexpr std::size_t N = 100000;
    constexpr int Q = 50000;

    std::cout << "\n[Ext1-Test 3] 查询占比50%负载 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(9999);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    int query_count = 0, update_count = 0, insert_count = 0, erase_count = 0;

    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 2;

        if (type == 0 && cur_len > 0) {
            // 50% 查询
            std::size_t l = rng() % cur_len;
            std::size_t r = rng() % cur_len;
            if (l > r) std::swap(l, r);
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
            query_count++;
        } else if (cur_len > 0) {
            // 50% 变更
            int change_type = rng() % 3;
            if (change_type == 0) {
                std::size_t idx = rng() % cur_len;
                dk.update(idx, val_dist(rng));
                update_count++;
            } else if (change_type == 1 && cur_len < N + 1000) {
                std::size_t idx = rng() % (cur_len + 1);
                dk.insert(idx, val_dist(rng));
                cur_len++;
                insert_count++;
            } else if (cur_len > 1) {
                std::size_t idx = rng() % cur_len;
                dk.erase(idx);
                cur_len--;
                erase_count++;
            }
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> 操作统计: 查询=" << query_count << ", 修改=" << update_count
              << ", 插入=" << insert_count << ", 删除=" << erase_count << std::endl;
    std::cout << "  -> " << Q << " 次混合操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 查询占比50%测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 6.4: 不同查询区间长度及排名
double test_ext1_various_ranges() {
    constexpr std::size_t N = 100000;
    constexpr int Q = 50000;

    std::cout << "\n[Ext1-Test 4] 不同查询区间长度及排名 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(1111);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        // 交替测试不同区间长度
        int range_type = op % 4;
        std::size_t l, r, k;

        if (range_type == 0) {
            // 小区间 (长度 1-10)
            l = rng() % cur_len;
            r = l + (rng() % 10);
            if (r >= cur_len) r = cur_len - 1;
            k = (rng() % (r - l + 1)) + 1;
        } else if (range_type == 1) {
            // 中等区间 (长度 100-1000)
            l = rng() % cur_len;
            r = l + 100 + (rng() % 900);
            if (r >= cur_len) r = cur_len - 1;
            k = (rng() % (r - l + 1)) + 1;
        } else if (range_type == 2) {
            // 大区间 (长度 10000-50000)
            l = rng() % cur_len;
            r = l + 10000 + (rng() % 40000);
            if (r >= cur_len) r = cur_len - 1;
            k = (rng() % (r - l + 1)) + 1;
        } else {
            // 全区间
            l = 0;
            r = cur_len - 1;
            k = (rng() % (r - l + 1)) + 1;
        }

        int res = dk.kthLargest(l, r, k);
        (void)res;
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次不同区间查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 不同查询区间长度及排名测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// -----------------------------------------------------------------------------
// 7. 拓展2测试 (N=1000000, Q=1000000)
// -----------------------------------------------------------------------------

// 测试 7.1: 查询主导负载
double test_ext2_query_dominant() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 1] 查询主导负载 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(1337);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        std::size_t l = rng() % cur_len;
        std::size_t r = rng() % cur_len;
        if (l > r) std::swap(l, r);
        std::size_t k = (rng() % (r - l + 1)) + 1;
        int res = dk.kthLargest(l, r, k);
        (void)res;
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 查询主导测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.2: 变更主导负载
double test_ext2_update_dominant() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 2] 变更主导负载 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(2024);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 10;
        if (type < 5 && cur_len > 0) {
            // 50% 修改
            std::size_t idx = rng() % cur_len;
            dk.update(idx, val_dist(rng));
        } else if (type < 7 && cur_len < N + 1000) {
            // 20% 插入
            std::size_t idx = rng() % (cur_len + 1);
            dk.insert(idx, val_dist(rng));
            cur_len++;
        } else if (cur_len > 1) {
            // 30% 删除
            std::size_t idx = rng() % cur_len;
            dk.erase(idx);
            cur_len--;
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次变更操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 变更主导测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.3: 查询占比 50% 负载
double test_ext2_mixed_50() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 3] 查询占比50%负载 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(9999);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    int query_count = 0, update_count = 0, insert_count = 0, erase_count = 0;

    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 2;

        if (type == 0 && cur_len > 0) {
            // 50% 查询
            std::size_t l = rng() % cur_len;
            std::size_t r = rng() % cur_len;
            if (l > r) std::swap(l, r);
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
            query_count++;
        } else if (cur_len > 0) {
            // 50% 变更
            int change_type = rng() % 3;
            if (change_type == 0) {
                std::size_t idx = rng() % cur_len;
                dk.update(idx, val_dist(rng));
                update_count++;
            } else if (change_type == 1 && cur_len < N + 1000) {
                std::size_t idx = rng() % (cur_len + 1);
                dk.insert(idx, val_dist(rng));
                cur_len++;
                insert_count++;
            } else if (cur_len > 1) {
                std::size_t idx = rng() % cur_len;
                dk.erase(idx);
                cur_len--;
                erase_count++;
            }
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> 操作统计: 查询=" << query_count << ", 修改=" << update_count
              << ", 插入=" << insert_count << ", 删除=" << erase_count << std::endl;
    std::cout << "  -> " << Q << " 次混合操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 查询占比50%测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.4: 聚簇与双峰值域
double test_ext2_clustered_bimodal() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 4] 聚簇与双峰值域 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(5678);
    auto initial_data = generate_clustered_bimodal_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        std::size_t l = rng() % cur_len;
        std::size_t r = rng() % cur_len;
        if (l > r) std::swap(l, r);
        std::size_t k = (rng() % (r - l + 1)) + 1;
        int res = dk.kthLargest(l, r, k);
        (void)res;
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 聚簇与双峰值域测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.5: 长区间中间排名
double test_ext2_long_range_mid_rank() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 5] 长区间中间排名 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(7777);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        // 长区间：长度在 100000-500000 之间
        std::size_t l = rng() % cur_len;
        std::size_t r = l + 100000 + (rng() % 400000);
        if (r >= cur_len) r = cur_len - 1;
        std::size_t range_len = r - l + 1;
        // 中间排名：k 在区间长度的 40%-60% 之间
        std::size_t mid_range = static_cast<std::size_t>(range_len * 0.2);
        if (mid_range == 0) mid_range = 1;
        std::size_t k = static_cast<std::size_t>(range_len * 0.4) + (rng() % mid_range);
        if (k < 1) k = 1;
        if (k > range_len) k = range_len;
        int res = dk.kthLargest(l, r, k);
        (void)res;
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次长区间中间排名查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 长区间中间排名测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.6: 窄初始值域后的宽域更新
double test_ext2_narrow_to_wide() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 6] 窄初始值域后的宽域更新 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(8888);
    auto initial_data = generate_narrow_range_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 2;
        if (type == 0 && cur_len > 0) {
            // 50% 查询
            std::size_t l = rng() % cur_len;
            std::size_t r = rng() % cur_len;
            if (l > r) std::swap(l, r);
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
        } else if (cur_len > 0) {
            // 50% 宽域更新
            std::size_t idx = rng() % cur_len;
            dk.update(idx, val_dist(rng));
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 窄初始值域后的宽域更新测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.7: 序列增长与收缩
double test_ext2_growth_shrink() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 7] 序列增长与收缩 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(3333);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 10;
        if (type < 3 && cur_len < N + 10000) {
            // 30% 插入（增长）
            std::size_t idx = rng() % (cur_len + 1);
            dk.insert(idx, val_dist(rng));
            cur_len++;
        } else if (type < 5 && cur_len > N - 10000) {
            // 20% 删除（收缩）
            std::size_t idx = rng() % cur_len;
            dk.erase(idx);
            cur_len--;
        } else if (cur_len > 0) {
            // 50% 查询
            std::size_t l = rng() % cur_len;
            std::size_t r = rng() % cur_len;
            if (l > r) std::swap(l, r);
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 序列增长与收缩测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.8: 分阶段热点迁移
double test_ext2_hotspot_migration() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 8] 分阶段热点迁移 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(4444);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        // 分阶段：每 1/3 操作查询不同区域
        int phase = (op * 3) / Q;
        std::size_t region_start, region_end;

        if (phase == 0) {
            // 前 1/3：查询前 1/3 区域
            region_start = 0;
            region_end = cur_len / 3;
        } else if (phase == 1) {
            // 中 1/3：查询中间 1/3 区域
            region_start = cur_len / 3;
            region_end = cur_len * 2 / 3;
        } else {
            // 后 1/3：查询后 1/3 区域
            region_start = cur_len * 2 / 3;
            region_end = cur_len;
        }

        if (region_start < region_end) {
            std::size_t l = region_start + (rng() % (region_end - region_start));
            std::size_t r = region_start + (rng() % (region_end - region_start));
            if (l > r) std::swap(l, r);
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次分阶段热点迁移查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 分阶段热点迁移测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.9: 反复插删
double test_ext2_repeated_insert_erase() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;

    std::cout << "\n[Ext2-Test 9] 反复插删 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(6666);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::uniform_int_distribution<int> val_dist(-1000000000, 1000000000);
    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        int type = rng() % 10;
        if (type < 4 && cur_len < N + 1000) {
            // 40% 插入
            std::size_t idx = rng() % (cur_len + 1);
            dk.insert(idx, val_dist(rng));
            cur_len++;
        } else if (type < 8 && cur_len > 1) {
            // 40% 删除
            std::size_t idx = rng() % cur_len;
            dk.erase(idx);
            cur_len--;
        } else if (cur_len > 0) {
            // 20% 查询
            std::size_t l = rng() % cur_len;
            std::size_t r = rng() % cur_len;
            if (l > r) std::swap(l, r);
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次反复插删操作耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 反复插删测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// 测试 7.10: 块边界
double test_ext2_block_boundary() {
    constexpr std::size_t N = 1000000;
    constexpr int Q = 1000000;
    constexpr std::size_t BLOCK_SIZE = 64;  // 假设块大小为 64

    std::cout << "\n[Ext2-Test 10] 块边界 (N=" << N << ", Q=" << Q << ")..." << std::endl;

    std::mt19937 rng(9999);
    auto initial_data = generate_initial_data(N, rng);

    auto start = std::chrono::high_resolution_clock::now();
    DynamicKth dk(initial_data);
    auto end = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> 构建耗时: " << build_ms << " ms" << std::endl;

    std::size_t cur_len = N;
    auto start_ops = std::chrono::high_resolution_clock::now();

    for (int op = 0; op < Q; ++op) {
        // 查询区间对齐块边界
        std::size_t num_blocks = cur_len / BLOCK_SIZE;
        if (num_blocks > 1) {
            std::size_t block_l = rng() % num_blocks;
            std::size_t block_r = rng() % num_blocks;
            if (block_l > block_r) std::swap(block_l, block_r);
            std::size_t l = block_l * BLOCK_SIZE;
            std::size_t r = (block_r + 1) * BLOCK_SIZE - 1;
            if (r >= cur_len) r = cur_len - 1;
            std::size_t k = (rng() % (r - l + 1)) + 1;
            int res = dk.kthLargest(l, r, k);
            (void)res;
        }
    }

    auto end_ops = std::chrono::high_resolution_clock::now();
    double ops_ms = std::chrono::duration<double, std::milli>(end_ops - start_ops).count();

    std::cout << "  -> " << Q << " 次块边界查询耗时: " << ops_ms << " ms" << std::endl;
    std::cout << "  ==> 块边界测试通过！" << std::endl;

    return build_ms + ops_ms;
}

// -----------------------------------------------------------------------------
// 8. 主函数入口
// -----------------------------------------------------------------------------
int main() {
    try {
        test_basic();
        test_small_stress(5000);
        test_basic_edge_cases();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  开始拓展1测试 (N=100,000, Q=50,000)" << std::endl;
        std::cout << "========================================" << std::endl;

        double ext1_total = 0.0;
        ext1_total += test_ext1_query_dominant();
        ext1_total += test_ext1_update_dominant();
        ext1_total += test_ext1_mixed_50();
        ext1_total += test_ext1_various_ranges();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  拓展1总耗时: " << std::fixed << std::setprecision(2) << ext1_total << " ms" << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << "\n========================================" << std::endl;
        std::cout << "  开始拓展2测试 (N=1,000,000, Q=1,000,000)" << std::endl;
        std::cout << "========================================" << std::endl;

        double ext2_total = 0.0;
        ext2_total += test_ext2_query_dominant();
        ext2_total += test_ext2_update_dominant();
        ext2_total += test_ext2_mixed_50();
        ext2_total += test_ext2_clustered_bimodal();
        ext2_total += test_ext2_long_range_mid_rank();
        ext2_total += test_ext2_narrow_to_wide();
        ext2_total += test_ext2_growth_shrink();
        ext2_total += test_ext2_hotspot_migration();
        ext2_total += test_ext2_repeated_insert_erase();
        ext2_total += test_ext2_block_boundary();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  拓展2总耗时: " << std::fixed << std::setprecision(2) << ext2_total << " ms" << std::endl;
        std::cout << "========================================" << std::endl;

        double total_time = ext1_total + ext2_total;

        std::cout << "\n========================================" << std::endl;
        std::cout << "  所有测试通过！(All Passed)" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  总耗时: " << std::fixed << std::setprecision(2) << total_time << " ms" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "捕获到异常: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
