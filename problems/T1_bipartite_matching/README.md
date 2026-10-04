# T1：带权二分图最大权匹配

## 题目描述

给定一个静态二分图。图的左部有 <code>left_count</code> 个顶点，右部有
<code>right_count</code> 个顶点；每条允许使用的边连接一个左部顶点和一个右部顶点，
并带有一个整数权重。

当图中至少有一条边时，请选出一个**非空匹配**，使每个顶点至多出现在一条
被选中的边中，并使被选边的权重总和最大。不存在于输入边集合中的顶点对不能匹配。

本题只要求最大化总权重，不要求匹配尽可能多。**空匹配的目标值视为负无穷**，
有边时不能返回空匹配。若所有边权都为负数，应选择权重最大的单条边；若最大
边权为零，也必须至少选择一条零权边。若存在多个总权重相同的最优非空匹配，
**返回其中任意一种即可**；测评不会要求返回某一组固定的边。

如果输入没有任何边，则不存在可行的非空匹配，返回全 <code>-1</code> 表示无解。
测评单独检查这种情况，不把该返回值解释为总分为零的匹配。

前后帧对象关联只是这个问题的一种应用场景，本题接口和题面不依赖帧、
时间或具体业务领域。

## 输入语义

顶点编号从 **0** 开始。

- 左部顶点编号为 <code>[0, left_count)</code>。
- 右部顶点编号为 <code>[0, right_count)</code>。
- <code>edges</code> 是允许使用的边集合。
- 测试数据保证每个 <code>Edge</code> 的端点编号合法，且同一个
  <code>(left, right)</code> 至多出现一次。
- 单条边的 <code>weight</code> 范围为 <code>[-10^9, 10^9]</code>，接口类型为
  <code>std::int64_t</code>，可以为负数、零或正数。
- 匹配总分应使用 <code>std::int64_t</code> 累加。最多选 2000 条边，总分绝对值
  不超过 <code>2 × 10^12</code>，可能超出 32 位整数范围。
- <code>left_count</code>、<code>right_count</code> 和 <code>edges</code> 都可能为空。

例如，若选择边 <code>(0, 2)</code> 和 <code>(3, 1)</code>，则左顶点
0、3 以及右顶点 2、1 各只出现一次，这两条边构成一个合法匹配。

## 返回值

<code>maximumWeightMatching()</code> 返回一个长度恰好为 <code>left_count</code> 的数组
<code>answer</code>：

- <code>answer[i] == -1</code>：左顶点 <code>i</code> 不匹配；
- <code>0 <= answer[i] < right_count</code>：左顶点 <code>i</code> 匹配到该右顶点。

返回结果必须同时满足：

1. 每个右顶点至多被一个左顶点使用；
2. 每个 <code>answer[i] != -1</code> 的 <code>(i, answer[i])</code> 都是输入中的允许边；
3. 输入有边时至少选择一条边，且这些边的权重总和达到最大可能总权重；
4. 输入无边时，所有元素必须为 <code>-1</code>，表示没有可行的非空匹配。

测评程序会检查上述合法性，并根据返回的边重新计算总权重。接口没有要求
考生额外返回总权重，也不会比较具体的匹配边集合。

## 示例

下面的图包含两个不同的最优匹配：

| 左顶点 | 右顶点 | 权重 |
|---:|---:|---:|
| 0 | 0 | 5 |
| 0 | 1 | 5 |
| 1 | 0 | 5 |
| 1 | 1 | 5 |
| 2 | 2 | -7 |

最大总权重为 <code>10</code>。以下两个返回值都正确：

~~~text
[0, 1, -1]   // 选择 (0,0)、(1,1)
[1, 0, -1]   // 选择 (0,1)、(1,0)
~~~

第三个左顶点不应选择权重为 <code>-7</code> 的边，因为空着它可以得到更大的总权重。
上述匹配已经含有两条边，满足非空约束。

如果输入只有 <code>(0,0,-5)</code>、<code>(1,1,-3)</code> 两条边，最大非空匹配
总分为 <code>-3</code>，应返回 <code>[-1,1]</code>。返回全 <code>-1</code> 会被判错。
非空约束也不等于要求最大匹配数量：若边权分别为 <code>(0,0,100)</code>、
<code>(0,1,1)</code>、<code>(1,0,1)</code>，最优答案只选择得分为 100 的一条边。

## 固定接口

接口定义在 <code>include/bipartite_matching.hpp</code> 中：

~~~cpp
struct Edge {
    std::uint32_t left;
    std::uint32_t right;
    std::int64_t weight;
};

class BipartiteMatcher {
public:
    BipartiteMatcher(std::uint32_t left_count,
                     std::uint32_t right_count,
                     const std::vector<Edge>& edges);
    ~BipartiteMatcher();

    [[nodiscard]] std::vector<std::int32_t> maximumWeightMatching() const;
};
~~~

构造函数会复制或读取输入数据；调用方不要求在对象外继续维护
<code>edges</code> 的存储。一个对象只对应一张图，<code>maximumWeightMatching()</code>
可以按接口约定被调用。

类名、公开函数签名、返回值含义和头文件中的对象布局必须保持不变。实现代码
放在 <code>src/</code> 下，可以增加辅助的 <code>.cpp</code> 或头文件。正式测评机使用自己的
CMake 配置和官方接口头文件，考生提交的 <code>main.cpp</code> 与
<code>CMakeLists.txt</code> 不参与正式测评。

## 数据规模与评分

<code>L</code> 表示 <code>left_count</code>，<code>R</code> 表示 <code>right_count</code>，
<code>E</code> 表示允许边数。所有拓展测试点都使用同一份编译结果；拓展之间互不构成前置条件。

| 档位 | 测试点数 | 规模与图形态 | 计分方式 |
| --- | ---: | --- | --- |
| 基础 | 10 | <code>L &lt;= 8</code>，<code>R &lt;= 8</code>，<code>E &lt;= L * R</code>，包含边界和多解情况 | 全部通过后才运行拓展 |
| 拓展 1 | 5 | <code>L &lt;= 200</code>，<code>R &lt;= 200</code>，稠密图 | 每点 2 分，共 10 分 |
| 拓展 2 | 5 | <code>L &lt;= 500</code>，<code>R &lt;= 500</code>，稠密图 | 每点 4 分，共 20 分 |
| 拓展 3 | 5 | <code>L &lt;= 2000</code>，<code>R &lt;= 2000</code>，<code>E &lt;= 50000</code>，稀疏图 | 每点 4 分，共 20 分 |

基础部分是前置条件：基础 10 个测试点未全部通过时，拓展测试点标记为
<code>SKIPPED</code>，不运行也不计分。基础全部通过后，拓展 1、拓展 2、拓展 3
使用同一次编译得到的程序分别测评；拓展 1 未通过不会阻止拓展 2 或拓展 3
继续测评。拓展部分总分为 <code>50</code> 分。

“稠密图”和“稀疏图”描述测试数据的边数分布；题目不向程序传入测试点
编号，也不要求根据测试点编号选择实现。所有合法输入都应由同一份代码处理，
不得依赖测评程序在不同拓展之间显式分派不同算法。

每个测试点的资源限制为墙钟时间 <code>3000 ms</code>、单核 CPU 和
<code>512 MiB</code> 虚拟地址空间；正式测评报告中的限制与版本公告优先。

## 测评流程

1. 测评系统使用官方接口头文件，并编译提交仓库
   <code>problems/T1_bipartite_matching/src/</code> 下的全部 <code>.cpp</code>。
2. 每个测试点构造一个 <code>BipartiteMatcher</code>，传入一张图并调用
   <code>maximumWeightMatching()</code>。
3. 测评程序检查返回数组长度、右侧顶点是否重复、返回边是否存在以及
   有边时是否非空，以及总权重是否为该测试点的最优总权重。
4. 只要返回匹配的总权重等于标准答案，且匹配本身合法，即使与标准答案的
   边集合不同，也判定该测试点正确。
5. 编译失败、链接失败、运行超时、崩溃、异常退出或返回非法匹配均不通过。

本题不规定必须使用某一种算法。实现应在基础、稠密拓展和稀疏拓展的全部
合法输入上保持正确；算法选择和数据结构由考生自行决定。

## 本地编译

在模板仓库根目录执行：

~~~bash
cmake -S problems/T1_bipartite_matching \
      -B /tmp/pip27-t1-build \
      -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/pip27-t1-build
~~~

模板中的占位实现只返回全 <code>-1</code>，作用是展示接口和保证工程可以编译，
只能处理无边输入，不能通过任何有边的正式测试。
