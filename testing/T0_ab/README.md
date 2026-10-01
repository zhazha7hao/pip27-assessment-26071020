# T0 A+B 公开测评示例

本目录保存 T0 的测评程序和测评用 CMake 配置，`../common/` 保存逐测试点运行及资源限制的共用实现。测评程序用自己的 `include/ab.hpp` 编译接口调用，从提交仓库的 `problems/T0_ab/src/` 递归收集全部 `.cpp` 文件；不使用题目自带的 `CMakeLists.txt` 或 `main.cpp`。

在模板仓库根目录，可以用 Linux 和 CMake 直接运行这个示例：

```bash
cmake -S testing/T0_ab -B /tmp/pip27-t0-grader -DSUBMISSION_ROOT="$PWD"
cmake --build /tmp/pip27-t0-grader --target grader resource_runner_test
ctest --test-dir /tmp/pip27-t0-grader --output-on-failure
/tmp/pip27-t0-grader/grader
```

`grader.cpp` 生成 10 个测试点，逐点调用 `Solver::sum()`，并输出判定、耗时、峰值 RSS 和通过数。上述命令是在本机演示测评代码；正式测评在独立 Docker 容器内重新编译、运行，具体耗时与内存数值会随环境变化。
