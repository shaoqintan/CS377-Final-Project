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
    src/utils.cpp
)

# Add header files
set(HEADERS
    include/node.hpp
    include/gossip_node.hpp
    include/heartbeat_node.hpp
    include/network.hpp
    include/simulator.hpp
    include/utils.hpp
)

# Create executable
add_executable(failure_detection ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(failure_detection PRIVATE include)

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
    tests/test_network.cpp
    tests/test_gossip.cpp
    tests/test_heartbeat.cpp
)

target_link_libraries(failure_detection_tests
    PRIVATE
    GTest::gtest_main
    failure_detection
)

include(GoogleTest)
gtest_discover_tests(failure_detection_tests) 