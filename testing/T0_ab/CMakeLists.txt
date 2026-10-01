cmake_minimum_required(VERSION 3.22)
project(T0_ab_grader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

# 各题共用同一套隔离执行与资源限制实现。
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../common"
    "${CMAKE_CURRENT_BINARY_DIR}/common")

# 正式测评必须由总控脚本传入考生提交仓库的绝对路径。
if(NOT DEFINED SUBMISSION_ROOT OR SUBMISSION_ROOT STREQUAL "")
    message(FATAL_ERROR
        "必须指定 SUBMISSION_ROOT，例如 -DSUBMISSION_ROOT=/submission")
endif()

get_filename_component(SUBMISSION_ROOT "${SUBMISSION_ROOT}" ABSOLUTE)
set(PROBLEM_ROOT "${SUBMISSION_ROOT}/problems/T0_ab")
set(STUDENT_SOURCE_ROOT "${PROBLEM_ROOT}/src")
set(STUDENT_INCLUDE_ROOT "${PROBLEM_ROOT}/include")

if(NOT IS_DIRECTORY "${STUDENT_SOURCE_ROOT}")
    message(FATAL_ERROR "找不到考生源码目录: ${STUDENT_SOURCE_ROOT}")
endif()

# 只收集当前题目的 src/，不使用考生自己的 CMakeLists.txt、main.cpp
# 或其他题目的代码。考生新增的辅助 .cpp 也会被自动编译。
file(GLOB_RECURSE STUDENT_SOURCES CONFIGURE_DEPENDS
    "${STUDENT_SOURCE_ROOT}/*.cpp")
list(SORT STUDENT_SOURCES)

if(NOT STUDENT_SOURCES)
    message(FATAL_ERROR "考生 src/ 目录中没有找到 .cpp 文件")
endif()

message(STATUS "本次编译的考生源码:")
foreach(source IN LISTS STUDENT_SOURCES)
    message(STATUS "  ${source}")
endforeach()

# OBJECT 库让每个收集到的 .cpp 都独立编译，并将所有目标文件直接交给
# 最终测评可执行文件链接。
add_library(candidate OBJECT ${STUDENT_SOURCES})
target_compile_features(candidate PRIVATE cxx_std_17)
target_include_directories(candidate PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${STUDENT_INCLUDE_ROOT}")

# 测评程序和官方接口一起编译，再链接考生实现的全部目标文件。
add_executable(grader
    grader.cpp
    $<TARGET_OBJECTS:candidate>)
target_compile_features(grader PRIVATE cxx_std_17)
target_include_directories(grader PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(grader PRIVATE resource_runner)
