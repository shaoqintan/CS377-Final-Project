#include "simulator.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

Simulator::Simulator() {}

void Simulator::setup_gossip_network(int num_nodes) {
    cleanup_network();
    
    // Create nodes
    std::vector<std::string> peer_ids;
    for (int i = 0; i < num_nodes; ++i) {
        std::string node_id = "node_" + std::to_string(i);
        peer_ids.push_back(node_id);
    }
    
    // Initialize nodes with their peer lists
    for (int i = 0; i < num_nodes; ++i) {
        std::string node_id = "node_" + std::to_string(i);
        std::vector<std::string> node_peers = peer_ids;
        node_peers.erase(std::remove(node_peers.begin(), node_peers.end(), node_id), node_peers.end());
        
        auto node = std::make_shared<GossipNode>(node_id, node_peers);
        network.add_node(node_id, node);
        node->start();
    }
}

void Simulator::setup_heartbeat_network(int num_nodes) {
    cleanup_network();
    
    // Create master node
    auto master = std::make_shared<HeartbeatNode>("master", true);
    network.add_node("master", master);
    master->start();
    
    // Create worker nodes
    for (int i = 0; i < num_nodes - 1; ++i) {
        std::string node_id = "worker_" + std::to_string(i);
        auto worker = std::make_shared<HeartbeatNode>(node_id, false);
        network.add_node(node_id, worker);
        master->add_node(node_id);
        worker->start();
    }
}

void Simulator::cleanup_network() {
    // Stop all nodes
    for (const auto& [id, node] : network.get_nodes()) {
        node->stop();
    }
    network.reset_stats();
}

Simulator::TestResult Simulator::run_single_node_failure_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    
    // Wait for initial convergence
    wait_for_convergence(5000);
    
    // Simulate single node failure
    std::string failed_node = "node_0";
    auto start_time = std::chrono::high_resolution_clock::now();
    network.simulate_node_failure(failed_node);
    
    // Wait for failure detection
    wait_for_convergence(5000);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    TestResult result = collect_metrics("Single Node Failure");
    result.detection_time_ms = detection_time.count();
    
    cleanup_network();
    return result;
}

Simulator::TestResult Simulator::run_multiple_failures_test(int num_nodes, int num_failures) {
    setup_gossip_network(num_nodes);
    
    // Wait for initial convergence
    wait_for_convergence(5000);
    
    // Simulate multiple failures
    std::vector<std::string> failed_nodes;
    for (int i = 0; i < num_failures; ++i) {
        failed_nodes.push_back("node_" + std::to_string(i));
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    simulate_failures(failed_nodes);
    
    // Wait for failure detection
    wait_for_convergence(5000);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    TestResult result = collect_metrics("Multiple Failures");
    result.detection_time_ms = detection_time.count();
    
    cleanup_network();
    return result;
}

Simulator::TestResult Simulator::run_network_partition_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    
    // Wait for initial convergence
    wait_for_convergence(5000);
    
    // Create two partitions
    std::vector<std::string> partition1, partition2;
    for (int i = 0; i < num_nodes; ++i) {
        if (i < num_nodes / 2) {
            partition1.push_back("node_" + std::to_string(i));
        } else {
            partition2.push_back("node_" + std::to_string(i));
        }
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    network.simulate_network_partition(partition1, partition2, 5000);
    
    // Wait for partition detection
    wait_for_convergence(5000);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    TestResult result = collect_metrics("Network Partition");
    result.detection_time_ms = detection_time.count();
    
    cleanup_network();
    return result;
}

Simulator::TestResult Simulator::run_high_load_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    
    // Wait for initial convergence
    wait_for_convergence(5000);
    
    // Simulate high load by sending many messages
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Send messages between random nodes for 5 seconds
    for (int i = 0; i < 1000; ++i) {
        int from = rand() % num_nodes;
        int to = rand() % num_nodes;
        if (from != to) {
            network.send_message("node_" + std::to_string(from),
                               "node_" + std::to_string(to),
                               "test_message");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto test_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    TestResult result = collect_metrics("High Load");
    result.detection_time_ms = test_time.count();
    
    cleanup_network();
    return result;
}

Simulator::TestResult Simulator::run_recovery_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    
    // Wait for initial convergence
    wait_for_convergence(5000);
    
    // Simulate node failure and recovery
    std::string node_id = "node_0";
    auto start_time = std::chrono::high_resolution_clock::now();
    
    network.simulate_node_failure(node_id);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    network.simulate_node_recovery(node_id);
    
    // Wait for recovery detection
    wait_for_convergence(5000);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    TestResult result = collect_metrics("Node Recovery");
    result.detection_time_ms = detection_time.count();
    
    cleanup_network();
    return result;
}

std::vector<Simulator::TestResult> Simulator::compare_algorithms(int num_nodes) {
    std::vector<TestResult> results;
    
    // Test gossip algorithm
    setup_gossip_network(num_nodes);
    results.push_back(run_single_node_failure_test(num_nodes));
    cleanup_network();
    
    // Test heartbeat algorithm
    setup_heartbeat_network(num_nodes);
    results.push_back(run_single_node_failure_test(num_nodes));
    cleanup_network();
    
    return results;
}

void Simulator::run_all_tests(int num_nodes) {
    std::cout << "Running all tests with " << num_nodes << " nodes...\n";
    
    auto results = compare_algorithms(num_nodes);
    
    std::cout << "\nTest Results:\n";
    for (const auto& result : results) {
        std::cout << "\nTest: " << result.test_name << "\n"
                  << "Detection Time: " << result.detection_time_ms << "ms\n"
                  << "False Positives: " << result.false_positives << "\n"
                  << "False Negatives: " << result.false_negatives << "\n"
                  << "Messages Sent: " << result.messages_sent << "\n"
                  << "Accuracy: " << (result.accuracy * 100) << "%\n";
    }
}

void Simulator::wait_for_convergence(int timeout_ms) {
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!check_convergence()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_time).count() > timeout_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Simulator::check_convergence() {
    // Implementation depends on the specific algorithm being tested
    // For now, we'll use a simple timeout-based approach
    return true;
}

void Simulator::simulate_failures(const std::vector<std::string>& node_ids) {
    for (const auto& node_id : node_ids) {
        network.simulate_node_failure(node_id);
    }
}

void Simulator::simulate_recoveries(const std::vector<std::string>& node_ids) {
    for (const auto& node_id : node_ids) {
        network.simulate_node_recovery(node_id);
    }
}

Simulator::TestResult Simulator::collect_metrics(const std::string& test_name) {
    TestResult result;
    result.test_name = test_name;
    
    // Collect network statistics
    auto net_stats = network.get_stats();
    result.messages_sent = net_stats.total_messages;
    
    // Collect algorithm-specific metrics
    // This would need to be implemented based on the specific algorithm being tested
    
    return result;
}

double Simulator::calculate_accuracy(int true_positives, int false_positives, int false_negatives) {
    int total = true_positives + false_positives + false_negatives;
    return total > 0 ? static_cast<double>(true_positives) / total : 0.0;
} 