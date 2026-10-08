#include "dynamic_kth.hpp"

#include <algorithm>

// 在此定义存储结构并实现算法；初始空结构仅保证模板可以编译。
struct DynamicKth::Impl {
    static constexpr std::size_t size = 9000;
    struct block {
        std::vector<int> elems;
        std::vector<int> sorted_elems;
        void insert(int pos, int value) {
            elems.insert(elems.begin() + pos, value);
            sorted_elems.push_back(value);
            std::sort(sorted_elems.begin(), sorted_elems.end());
        }
        void build()
        {
            sorted_elems = elems;
            std::sort(sorted_elems.begin(), sorted_elems.end());
        }
        void erase(int pos) {
            int val = elems[pos];
            elems.erase(elems.begin() + pos);
            auto it = std::find(sorted_elems.begin(), sorted_elems.end(), val);
            sorted_elems.erase(it);
        }
        std::size_t count(int val) const {
            return sorted_elems.end() - std::lower_bound(sorted_elems.begin(), sorted_elems.end(), val);
        }
        std::size_t count_range(std::size_t l, std::size_t r, int val) const {
            std::size_t cnt = 0;
            for (std::size_t i = l; i <= r; ++i) {
                if (elems[i] >= val) ++cnt;
            }
            return cnt;
        }
    };
    void split(int pos)
    {
        if(blocks[pos].elems.size() <= 2*size) return;
        block new_block;
        int mid = blocks[pos].elems.size()/2;
        new_block.elems.assign(blocks[pos].elems.begin() + mid, blocks[pos].elems.end());
        blocks[pos].elems.erase(blocks[pos].elems.begin() + mid, blocks[pos].elems.end());
        new_block.build();
        blocks[pos].build();
        blocks.insert(blocks.begin() + pos + 1, std::move(new_block));
    }
    std::vector<block> blocks;
    std::size_t count_range(std::size_t block_id1, std::size_t block_id2, std::size_t l, std::size_t r, int val) const {
        if(block_id1 == block_id2) return blocks[block_id1].count_range(l, r, val);
        int sum=0;
        sum += blocks[block_id1].count_range(l, blocks[block_id1].elems.size() - 1, val);
        for (std::size_t i = block_id1 + 1; i < block_id2; i++) {
            sum+=blocks[i].count(val);
        }
        sum += blocks[block_id2].count_range(0, r, val);
        return sum;
    }
};

DynamicKth::DynamicKth(const std::vector<int>& initial)
    : impl_(std::make_unique<Impl>()) {
    // TODO: 用 `initial` 构造序列，允许初始序列为空
    std::size_t n = initial.size();
    for (std::size_t i = 0; i < n; i += Impl::size) {
        Impl::block block;
        std::size_t end = std::min(i + Impl::size, n);
        block.elems.assign(initial.begin() + i, initial.begin() + end);
        block.build();
        impl_->blocks.push_back(std::move(block));
    }
}

DynamicKth::~DynamicKth() = default;

void DynamicKth::insert(std::size_t index, int value) {
    // TODO: 在当前 `index` 位置之前插入；`index == 当前长度` 表示末尾追加
    if (impl_->blocks.empty()) {
        impl_->blocks.emplace_back().insert(0, value);
        return;
    }
    int cnt =0;
    for(std::size_t i=0;i<impl_->blocks.size();i++){
        if(index <= (cnt + impl_->blocks[i].elems.size())){
            impl_->blocks[i].insert(index-cnt,value);
            impl_->split(i);
            return;
        }
        cnt += impl_->blocks[i].elems.size();
    }
    impl_->blocks.back().insert(index-cnt,value);
    impl_->split(impl_->blocks.size() - 1);
    return;
}

void DynamicKth::erase(std::size_t index) {
    // TODO: 删除当前 `index` 位置的元素
    int cnt =0;
    for(std::size_t i=0;i<impl_->blocks.size();i++){
        if(index < (cnt + impl_->blocks[i].elems.size())){
            impl_->blocks[i].erase(index - cnt);
            if (impl_->blocks[i].elems.empty()) {
                impl_->blocks.erase(impl_->blocks.begin() + i);
            }
            return;
        }
        cnt += impl_->blocks[i].elems.size();
    }
    return;
}

void DynamicKth::update(std::size_t index, int value) {
    // TODO: 将当前 `index` 位置的元素修改为 `value`
    int cnt =0;
    for(std::size_t i=0;i<impl_->blocks.size();i++){
        if(index < (cnt + impl_->blocks[i].elems.size())){
            impl_->blocks[i].elems[index - cnt] = value;
            impl_->blocks[i].build();
            return;
        }
        cnt += impl_->blocks[i].elems.size();
    }
}

int DynamicKth::kthLargest(std::size_t left, std::size_t right, std::size_t k) const {
    // TODO: 返回当前闭区间 `[left, right]` 中第 `k` 大的值，`k` 从 **1** 开始
    int id1,id2,l,r;
    int cnt = 0,ans;
    for(std::size_t i=0;i<impl_->blocks.size();i++){
        if(left < (cnt + impl_->blocks[i].elems.size())){
            l=left-cnt;
            id1=i;
            break;
        }
        cnt += impl_->blocks[i].elems.size();
    }
    cnt = 0;
    for(std::size_t i=0;i<impl_->blocks.size();i++){
        if(right < (cnt + impl_->blocks[i].elems.size())){
            r=right-cnt;
            id2=i;
            break;
        }
        cnt += impl_->blocks[i].elems.size();
    }
    int low=-1e9,high=1e9;
    ans=high;
    while(low <= high)
    {
        long long mid = low + (high - low) / 2;
        if (impl_->count_range(id1,id2,l, r, static_cast<int>(mid)) >= k) {
            ans = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return ans;
}