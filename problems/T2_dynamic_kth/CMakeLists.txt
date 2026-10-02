cmake_minimum_required(VERSION 3.22)
project(T2_dynamic_kth LANGUAGES CXX)

# 自动编译本题 src/ 下的全部 .cpp，包括考生新增的实现文件。
file(GLOB_RECURSE T2_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 题目实现不提供 main()，由本地演示程序或正式测评机调用固定接口。
add_library(t2_dynamic_kth STATIC ${T2_SOURCES})
target_compile_features(t2_dynamic_kth PUBLIC cxx_std_17)
target_include_directories(t2_dynamic_kth PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 本地检查只验证基本接口行为；正式测评使用独立测试程序。
add_executable(t2_dynamic_kth_demo main.cpp)
target_link_libraries(t2_dynamic_kth_demo PRIVATE t2_dynamic_kth)
