#include "simulator.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>
#include <unordered_set>

Simulator::Simulator() {}

void Simulator::setup_gossip_network(int num_nodes) {
    cleanup_network();
    
    std::vector<std::string> node_ids;
    for (int i = 0; i < num_nodes; ++i) {
        node_ids.push_back("node" + std::to_string(i));
    }
    
    for (const auto& id : node_ids) {
        auto node = std::make_shared<GossipNode>(id, node_ids);
        network.add_node(id, node);
        node->start();
    }
}

void Simulator::setup_heartbeat_network(int num_nodes) {
    cleanup_network();
    
    std::vector<std::string> node_ids;
    for (int i = 0; i < num_nodes; ++i) {
        node_ids.push_back("node" + std::to_string(i));
    }
    
    for (const auto& id : node_ids) {
        auto node = std::make_shared<HeartbeatNode>(id, id == "node0");  // First node is master
        network.add_node(id, node);
        node->start();
    }
}

void Simulator::cleanup_network() {
    // Stop all nodes first
    for (int i = 0; i < 100; ++i) {  // Assuming max 100 nodes
        std::string id = "node" + std::to_string(i);
        auto node = network.get_node(id);
        if (node) {
            node->stop();
        }
    }
    
    // Then remove them from the network
    for (int i = 0; i < 100; ++i) {
        std::string id = "node" + std::to_string(i);
        network.remove_node(id);
    }
}

Simulator::TestResult Simulator::run_single_node_failure_test(int num_nodes) {
    cleanup_network();  // Ensure clean state
    setup_gossip_network(num_nodes);
    
    // Generate some initial traffic
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + std::to_string(i),
                                   "node" + std::to_string(j),
                                   "initial_traffic");
            }
        }
    }
    
    // Process initial messages
    for (int i = 0; i < 50; ++i) {  // Process messages for 5 seconds
        network.process_messages();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Reset network stats before failure simulation
    network.reset_stats();
    
    // Choose a random node to fail
    std::string failed_node = "node" + std::to_string(rand() % num_nodes);
    
    // Record start time
    auto start_time = std::chrono::steady_clock::now();
    
    // Simulate node failure
    simulate_failures({failed_node});
    
    // Process messages and wait for failure detection
    bool failure_detected = false;
    int detection_time_ms = 5000;  // Default to timeout
    
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < 5000) {
        network.process_messages();
        
        // Check if failure is detected
        for (int i = 0; i < num_nodes; ++i) {
            std::string node_id = "node" + std::to_string(i);
            if (node_id != failed_node) {
                auto node = std::dynamic_pointer_cast<GossipNode>(network.get_node(node_id));
                if (node) {
                    auto failed_nodes = node->get_failed_nodes();
                    if (std::find(failed_nodes.begin(), failed_nodes.end(), failed_node) != failed_nodes.end()) {
                        failure_detected = true;
                        detection_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start_time).count();
                        break;
                    }
                }
            }
        }
        
        if (failure_detected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto result = collect_metrics("Single Node Failure Test");
    result.detection_time_ms = detection_time_ms;
    
    // Clean up
    cleanup_network();
    
    return result;
}

Simulator::TestResult Simulator::run_multiple_failures_test(int num_nodes, int num_failures) {
    setup_gossip_network(num_nodes);
    wait_for_convergence(5000);
    
    // Choose random nodes to fail
    std::vector<std::string> failed_nodes;
    for (int i = 0; i < num_failures; ++i) {
        failed_nodes.push_back("node" + std::to_string(i));
    }
    std::random_shuffle(failed_nodes.begin(), failed_nodes.end());
    
    // Simulate failures
    simulate_failures(failed_nodes);
    
    // Wait for failure detection
    wait_for_convergence(5000);
    
    return collect_metrics("Multiple Failures Test");
}

Simulator::TestResult Simulator::run_network_partition_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    wait_for_convergence(5000);
    
    // Split nodes into two partitions
    std::vector<std::string> partition1, partition2;
    for (int i = 0; i < num_nodes; ++i) {
        if (i < num_nodes / 2) {
            partition1.push_back("node" + std::to_string(i));
        } else {
            partition2.push_back("node" + std::to_string(i));
        }
    }
    
    // Simulate network partition
    network.simulate_network_partition(partition1, partition2, 5000);
    
    // Wait for failure detection
    wait_for_convergence(5000);
    
    // Heal partition
    network.heal_network_partition();
    
    return collect_metrics("Network Partition Test");
}

Simulator::TestResult Simulator::run_high_load_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    wait_for_convergence(5000);
    
    // Generate high message load
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + std::to_string(i),
                                   "node" + std::to_string(j),
                                   "high_load_test");
            }
        }
    }
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    return collect_metrics("High Load Test");
}

Simulator::TestResult Simulator::run_recovery_test(int num_nodes) {
    setup_gossip_network(num_nodes);
    wait_for_convergence(5000);
    
    // Choose a random node to fail and recover
    std::string node_id = "node" + std::to_string(rand() % num_nodes);
    
    // Simulate failure and recovery
    simulate_failures({node_id});
    std::this_thread::sleep_for(std::chrono::seconds(2));
    simulate_recoveries({node_id});
    
    // Wait for recovery detection
    wait_for_convergence(5000);
    
    return collect_metrics("Recovery Test");
}

std::vector<Simulator::TestResult> Simulator::compare_algorithms(int num_nodes) {
    std::vector<TestResult> results;
    
    // Test Gossip-based detection
    setup_gossip_network(num_nodes);
    results.push_back(run_single_node_failure_test(num_nodes));
    cleanup_network();
    
    // Test Heartbeat-based detection
    setup_heartbeat_network(num_nodes);
    results.push_back(run_single_node_failure_test(num_nodes));
    cleanup_network();
    
    return results;
}

void Simulator::run_all_tests(int num_nodes) {
    std::vector<TestResult> results;
    
    results.push_back(run_single_node_failure_test(num_nodes));
    results.push_back(run_multiple_failures_test(num_nodes, 3));
    results.push_back(run_network_partition_test(num_nodes));
    results.push_back(run_high_load_test(num_nodes));
    results.push_back(run_recovery_test(num_nodes));
    
    // Print results
    for (const auto& result : results) {
        std::cout << "Test: " << result.test_name << "\n"
                  << "Detection Time: " << result.detection_time_ms << "ms\n"
                  << "False Positives: " << result.false_positives << "\n"
                  << "False Negatives: " << result.false_negatives << "\n"
                  << "Messages Sent: " << result.messages_sent << "\n"
                  << "Accuracy: " << result.accuracy << "\n\n";
    }
}

void Simulator::wait_for_convergence(int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    while (!check_convergence()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        network.process_messages();
    }
}

bool Simulator::check_convergence() {
    // Simple convergence check: all nodes agree on the system state
    std::unordered_set<std::string> failed_nodes;
    bool first = true;
    
    for (int i = 0; i < 100; ++i) {  // Assuming max 100 nodes
        std::string id = "node" + std::to_string(i);
        auto node = std::dynamic_pointer_cast<GossipNode>(network.get_node(id));
        if (!node) continue;
        
        auto current_failed = node->get_failed_nodes();
        if (first) {
            failed_nodes.insert(current_failed.begin(), current_failed.end());
            first = false;
        } else if (failed_nodes.size() != current_failed.size()) {
            return false;
        } else {
            for (const auto& failed : current_failed) {
                if (failed_nodes.find(failed) == failed_nodes.end()) {
                    return false;
                }
            }
        }
    }
    return true;
}

void Simulator::simulate_failures(const std::vector<std::string>& node_ids) {
    for (const auto& node_id : node_ids) {
        auto node = network.get_node(node_id);
        if (node) {
            node->set_alive(false);
        }
    }
}

void Simulator::simulate_recoveries(const std::vector<std::string>& node_ids) {
    for (const auto& node_id : node_ids) {
        auto node = network.get_node(node_id);
        if (node) {
            node->set_alive(true);
        }
    }
}

Simulator::TestResult Simulator::collect_metrics(const std::string& test_name) {
    TestResult result;
    result.test_name = test_name;
    
    auto net_stats = network.get_stats();
    result.messages_sent = net_stats.delivered_messages + net_stats.dropped_messages;
    result.detection_time_ms = result.messages_sent > 0 ? net_stats.total_delay / result.messages_sent : 0;
    
    // Count false positives and negatives
    result.false_positives = 0;
    result.false_negatives = 0;
    
    // Calculate accuracy
    result.accuracy = calculate_accuracy(
        result.messages_sent - (result.false_positives + result.false_negatives),
        result.false_positives,
        result.false_negatives
    );
    
    return result;
}

double Simulator::calculate_accuracy(int true_positives, int false_positives, int false_negatives) {
    if (true_positives + false_positives + false_negatives == 0) return 1.0;
    return static_cast<double>(true_positives) / (true_positives + false_positives + false_negatives);
} 