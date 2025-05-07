cmake_minimum_required(VERSION 3.10)
project(DistributedFailureDetection)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/node.cpp
    src/gossip_node.cpp
    src/heartbeat_node.cpp
    src/network.cpp
    src/simulator.cpp
)

# Add header files
set(HEADERS
    include/node.hpp
    include/gossip_node.hpp
    include/heartbeat_node.hpp
    include/network.hpp
    include/simulator.hpp
)

# Create library
add_library(failure_detection_lib STATIC ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(failure_detection_lib PUBLIC include)

# Create main executable
add_executable(failure_detection src/main.cpp)
target_link_libraries(failure_detection PRIVATE failure_detection_lib)

# Add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(failure_detection_tests
    tests/test_main.cpp
)

target_link_libraries(failure_detection_tests
    PRIVATE
    GTest::gtest_main
    failure_detection_lib
)

# Enable testing
enable_testing()
include(GoogleTest)
gtest_discover_tests(failure_detection_tests) 