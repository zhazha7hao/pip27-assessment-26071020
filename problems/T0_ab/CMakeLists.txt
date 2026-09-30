cmake_minimum_required(VERSION 3.22)
project(T0_ab LANGUAGES CXX)

# 自动编译本题 src/ 下的所有 .cpp，包括后续新增的实现文件。
file(GLOB_RECURSE AB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 题目实现编译为库；src/ 中不放 main()。
add_library(t0_ab STATIC ${AB_SOURCES})
target_compile_features(t0_ab PUBLIC cxx_std_17)
target_include_directories(t0_ab PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 本地运行入口，仅用于演示接口如何被调用，不负责正式评分。
add_executable(t0_ab_demo main.cpp)
target_link_libraries(t0_ab_demo PRIVATE t0_ab)