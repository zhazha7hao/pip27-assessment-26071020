add_library(resource_runner STATIC src/resource_runner.cpp)
target_compile_features(resource_runner PUBLIC cxx_std_17)
target_include_directories(resource_runner PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

# 独立行为测试仅在显式指定目标时编译，正式测评只编译 grader。
add_executable(resource_runner_test EXCLUDE_FROM_ALL tests/resource_runner_test.cpp)
target_link_libraries(resource_runner_test PRIVATE resource_runner)
add_test(NAME resource_runner_test COMMAND resource_runner_test)
