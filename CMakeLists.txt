cmake_minimum_required(VERSION 3.14)
project(stripe_gateway_pipeline CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)

# cpp-httplib
FetchContent_Declare(
    httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.15.3.zip
)

FetchContent_MakeAvailable(googletest nlohmann_json httplib)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_library(gateway_pipeline_lib ${SRC_FILES})
target_include_directories(gateway_pipeline_lib PUBLIC include)
target_link_libraries(gateway_pipeline_lib PUBLIC httplib::httplib nlohmann_json::nlohmann_json)

enable_testing()
add_executable(run_tests tests/unit_tests.cpp tests/integration_tests.cpp)
target_include_directories(run_tests PRIVATE include tests)
target_link_libraries(run_tests PRIVATE gateway_pipeline_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(run_tests)
