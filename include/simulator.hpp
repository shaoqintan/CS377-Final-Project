#pragma once

#include "network.hpp"
#include "gossip_node.hpp"
#include "heartbeat_node.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <functional>

class Simulator {
public:
    struct TestResult {
        std::string test_name;
        double detection_time_ms;
        int false_positives;
        int false_negatives;
        int messages_sent;
        double accuracy;
    };

    Simulator();
    ~Simulator() = default;

    // Test scenarios
    TestResult run_single_node_failure_test(int num_nodes);
    TestResult run_multiple_failures_test(int num_nodes, int num_failures);
    TestResult run_network_partition_test(int num_nodes);
    TestResult run_high_load_test(int num_nodes);
    TestResult run_recovery_test(int num_nodes);

    // Comparison tests
    std::vector<TestResult> compare_algorithms(int num_nodes);
    void run_all_tests(int num_nodes);

private:
    Network network;
    
    // Helper functions
    void setup_gossip_network(int num_nodes);
    void setup_heartbeat_network(int num_nodes);
    void cleanup_network();
    
    // Metrics collection
    TestResult collect_metrics(const std::string& test_name);
    double calculate_accuracy(int true_positives, int false_positives, int false_negatives);
    
    // Test utilities
    void wait_for_convergence(int timeout_ms);
    bool check_convergence();
    void simulate_failures(const std::vector<std::string>& node_ids);
    void simulate_recoveries(const std::vector<std::string>& node_ids);
}; 