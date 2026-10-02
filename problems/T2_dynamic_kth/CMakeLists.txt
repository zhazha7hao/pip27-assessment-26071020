cmake_minimum_required(VERSION 3.22)
project(T2_dynamic_kth LANGUAGES CXX)

# 自动编译本题 src/ 下的全部 .cpp，包括考生新增的实现文件。
file(GLOB_RECURSE T2_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 题目模板只编译待测实现；正式测评机提供独立的程序入口。
add_library(t2_dynamic_kth STATIC ${T2_SOURCES})
target_compile_features(t2_dynamic_kth PUBLIC cxx_std_17)
target_include_directories(t2_dynamic_kth PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
