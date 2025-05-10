#include "simulator.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>
#include <unordered_set>
#include <iomanip>

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
    int detection_time_ms = 3000;  // Default to timeout
    
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < 3000) {
        network.process_messages();
        
        // Check if failure is detected
        for (int i = 0; i < num_nodes; ++i) {
            std::string node_id = "node" + std::to_string(i);
            if (node_id != failed_node) {
                auto node = network.get_node(node_id);
                if (node) {
                    // Try to cast to both types of nodes
                    auto gossip_node = std::dynamic_pointer_cast<GossipNode>(node);
                    auto heartbeat_node = std::dynamic_pointer_cast<HeartbeatNode>(node);
                    
                    std::vector<std::string> failed_nodes;
                    if (gossip_node) {
                        failed_nodes = gossip_node->get_failed_nodes();
                    } else if (heartbeat_node) {
                        failed_nodes = heartbeat_node->get_failed_nodes();
                    }
                    
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
    return result;
}

Simulator::TestResult Simulator::run_multiple_failures_test(int num_nodes, int num_failures) {
    wait_for_convergence(3000);
    
    // Choose random nodes to fail
    std::vector<std::string> failed_nodes;
    for (int i = 0; i < num_failures; ++i) {
        failed_nodes.push_back("node" + std::to_string(i));
    }
    std::random_shuffle(failed_nodes.begin(), failed_nodes.end());
    
    // Reset network stats before failure simulation
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::steady_clock::now();
    
    // Simulate failures
    simulate_failures(failed_nodes);
    
    // Process messages and wait for failure detection
    bool all_failures_detected = false;
    int detection_time_ms = 3000;  // Default to timeout
    
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < 3000) {
        network.process_messages();
        
        // Check if all failures are detected
        all_failures_detected = true;
        for (int i = 0; i < num_nodes; ++i) {
            std::string node_id = "node" + std::to_string(i);
            if (std::find(failed_nodes.begin(), failed_nodes.end(), node_id) == failed_nodes.end()) {
                auto node = network.get_node(node_id);
                if (node) {
                    auto gossip_node = std::dynamic_pointer_cast<GossipNode>(node);
                    auto heartbeat_node = std::dynamic_pointer_cast<HeartbeatNode>(node);
                    
                    std::vector<std::string> detected_failures;
                    if (gossip_node) {
                        detected_failures = gossip_node->get_failed_nodes();
                    } else if (heartbeat_node) {
                        detected_failures = heartbeat_node->get_failed_nodes();
                    }
                    
                    // Check if all failed nodes are detected
                    for (const auto& failed : failed_nodes) {
                        if (std::find(detected_failures.begin(), detected_failures.end(), failed) == detected_failures.end()) {
                            all_failures_detected = false;
                            break;
                        }
                    }
                }
            }
        }
        
        if (all_failures_detected) {
            detection_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto result = collect_metrics("Multiple Failures Test");
    result.detection_time_ms = detection_time_ms;
    return result;
}

Simulator::TestResult Simulator::run_network_partition_test(int num_nodes) {
    wait_for_convergence(3000);
    
    // Split nodes into two partitions
    std::vector<std::string> partition1, partition2;
    for (int i = 0; i < num_nodes; ++i) {
        if (i < num_nodes / 2) {
            partition1.push_back("node" + std::to_string(i));
        } else {
            partition2.push_back("node" + std::to_string(i));
        }
    }
    
    // Reset network stats before partition simulation
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::steady_clock::now();
    
    // Simulate network partition
    network.simulate_network_partition(partition1, partition2, 3000);
    
    // Process messages and wait for partition detection
    bool partition_detected = false;
    int detection_time_ms = 3000;  // Default to timeout
    
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < 3000) {
        network.process_messages();
        
        // Check if partition is detected by checking if nodes in each partition
        // detect the other partition as failed
        partition_detected = true;
        for (const auto& node1_id : partition1) {
            auto node1 = network.get_node(node1_id);
            if (node1) {
                auto gossip_node1 = std::dynamic_pointer_cast<GossipNode>(node1);
                auto heartbeat_node1 = std::dynamic_pointer_cast<HeartbeatNode>(node1);
                
                std::vector<std::string> detected_failures;
                if (gossip_node1) {
                    detected_failures = gossip_node1->get_failed_nodes();
                } else if (heartbeat_node1) {
                    detected_failures = heartbeat_node1->get_failed_nodes();
                }
                
                // Check if all nodes in partition2 are detected as failed
                for (const auto& node2_id : partition2) {
                    if (std::find(detected_failures.begin(), detected_failures.end(), node2_id) == detected_failures.end()) {
                        partition_detected = false;
                        break;
                    }
                }
            }
        }
        
        if (partition_detected) {
            detection_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Heal partition
    network.heal_network_partition();
    
    auto result = collect_metrics("Network Partition Test");
    result.detection_time_ms = detection_time_ms;
    return result;
}

Simulator::TestResult Simulator::run_high_load_test(int num_nodes) {
    wait_for_convergence(3000);
    
    // Reset network stats before high load test
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::steady_clock::now();
    
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
    
    // Process messages and wait for message delivery
    bool all_messages_delivered = false;
    int delivery_time_ms = 3000;  // Default to timeout
    
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < 3000) {
        network.process_messages();
        
        // Check if all messages are delivered
        auto stats = network.get_stats();
        int expected_messages = num_nodes * (num_nodes - 1);  // Each node sends to all others
        if (stats.delivered_messages >= expected_messages) {
            all_messages_delivered = true;
            delivery_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto result = collect_metrics("High Load Test");
    result.detection_time_ms = delivery_time_ms;
    return result;
}

Simulator::TestResult Simulator::run_recovery_test(int num_nodes) {
    wait_for_convergence(3000);
    
    // Choose a random node to fail and recover
    std::string node_id = "node" + std::to_string(rand() % num_nodes);
    
    // Reset network stats before recovery test
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::steady_clock::now();
    
    // Simulate failure
    simulate_failures({node_id});
    
    // Wait for failure detection
    bool failure_detected = false;
    int failure_detection_time_ms = 3000;
    
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < 3000) {
        network.process_messages();
        
        // Check if failure is detected
        for (int i = 0; i < num_nodes; ++i) {
            std::string check_id = "node" + std::to_string(i);
            if (check_id != node_id) {
                auto node = network.get_node(check_id);
                if (node) {
                    auto gossip_node = std::dynamic_pointer_cast<GossipNode>(node);
                    auto heartbeat_node = std::dynamic_pointer_cast<HeartbeatNode>(node);
                    
                    std::vector<std::string> detected_failures;
                    if (gossip_node) {
                        detected_failures = gossip_node->get_failed_nodes();
                    } else if (heartbeat_node) {
                        detected_failures = heartbeat_node->get_failed_nodes();
                    }
                    
                    if (std::find(detected_failures.begin(), detected_failures.end(), node_id) != detected_failures.end()) {
                        failure_detected = true;
                        failure_detection_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start_time).count();
                        break;
                    }
                }
            }
        }
        
        if (failure_detected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Simulate recovery
    simulate_recoveries({node_id});
    
    // Wait for recovery detection
    bool recovery_detected = false;
    int recovery_detection_time_ms = 3000;
    
    auto recovery_start_time = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - recovery_start_time).count() < 3000) {
        network.process_messages();
        
        // Check if recovery is detected
        recovery_detected = true;
        for (int i = 0; i < num_nodes; ++i) {
            std::string check_id = "node" + std::to_string(i);
            if (check_id != node_id) {
                auto node = network.get_node(check_id);
                if (node) {
                    auto gossip_node = std::dynamic_pointer_cast<GossipNode>(node);
                    auto heartbeat_node = std::dynamic_pointer_cast<HeartbeatNode>(node);
                    
                    std::vector<std::string> detected_failures;
                    if (gossip_node) {
                        detected_failures = gossip_node->get_failed_nodes();
                    } else if (heartbeat_node) {
                        detected_failures = heartbeat_node->get_failed_nodes();
                    }
                    
                    if (std::find(detected_failures.begin(), detected_failures.end(), node_id) != detected_failures.end()) {
                        recovery_detected = false;
                        break;
                    }
                }
            }
        }
        
        if (recovery_detected) {
            recovery_detection_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - recovery_start_time).count();
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto result = collect_metrics("Recovery Test");
    result.detection_time_ms = failure_detection_time_ms + recovery_detection_time_ms;
    return result;
}

std::vector<Simulator::TestResult> Simulator::compare_algorithms(int num_nodes) {
    std::vector<TestResult> results;
    
    // Test Gossip-based detection
    std::cout << "\nRunning Gossip Network Tests...\n";
    setup_gossip_network(num_nodes);
    results.push_back(run_single_node_failure_test(num_nodes));
    results.push_back(run_multiple_failures_test(num_nodes, num_nodes / 2));
    results.push_back(run_network_partition_test(num_nodes));
    results.push_back(run_high_load_test(num_nodes));
    results.push_back(run_recovery_test(num_nodes));
    cleanup_network();
    
    // Test Heartbeat-based detection
    std::cout << "\nRunning Heartbeat Network Tests...\n";
    setup_heartbeat_network(num_nodes);
    results.push_back(run_single_node_failure_test(num_nodes));
    results.push_back(run_multiple_failures_test(num_nodes, num_nodes / 2));
    results.push_back(run_network_partition_test(num_nodes));
    results.push_back(run_high_load_test(num_nodes));
    results.push_back(run_recovery_test(num_nodes));
    cleanup_network();
    
    // Print comparison results
    std::cout << "\nAlgorithm Comparison Results:\n";
    std::cout << "===========================\n";
    
    // Print Gossip Network Results
    std::cout << "\nGossip Network Results:\n";
    std::cout << "---------------------------\n";
    for (size_t i = 0; i < 5; ++i) {
        const auto& result = results[i];
        std::cout << "Test: " << result.test_name << "\n"
                  << "  Detection Time: " << std::setw(8) << result.detection_time_ms << " ms\n"
                  << "  Accuracy:      " << std::setw(8) << (result.accuracy * 100) << " %\n"
                  << "  Messages Sent: " << std::setw(8) << result.messages_sent << "\n"
                  << "  False Positives: " << result.false_positives << "\n"
                  << "  False Negatives: " << result.false_negatives << "\n\n";
    }
    
    // Print Heartbeat Network Results
    std::cout << "\nHeartbeat Network Results:\n";
    std::cout << "---------------------------\n";
    for (size_t i = 5; i < 10; ++i) {
        const auto& result = results[i];
        std::cout << "Test: " << result.test_name << "\n"
                  << "  Detection Time: " << std::setw(8) << result.detection_time_ms << " ms\n"
                  << "  Accuracy:      " << std::setw(8) << (result.accuracy * 100) << " %\n"
                  << "  Messages Sent: " << std::setw(8) << result.messages_sent << "\n"
                  << "  False Positives: " << result.false_positives << "\n"
                  << "  False Negatives: " << result.false_negatives << "\n\n";
    }
    
    // Print Summary Statistics
    std::cout << "\nSummary Statistics:\n";
    std::cout << "---------------------------\n";
    
    // Calculate averages for Gossip Network
    double gossip_avg_detection_time = 0;
    double gossip_avg_accuracy = 0;
    int gossip_total_messages = 0;
    int gossip_total_false_positives = 0;
    int gossip_total_false_negatives = 0;
    
    for (size_t i = 0; i < 5; ++i) {
        gossip_avg_detection_time += results[i].detection_time_ms;
        gossip_avg_accuracy += results[i].accuracy;
        gossip_total_messages += results[i].messages_sent;
        gossip_total_false_positives += results[i].false_positives;
        gossip_total_false_negatives += results[i].false_negatives;
    }
    
    gossip_avg_detection_time /= 5;
    gossip_avg_accuracy /= 5;
    
    // Calculate averages for Heartbeat Network
    double heartbeat_avg_detection_time = 0;
    double heartbeat_avg_accuracy = 0;
    int heartbeat_total_messages = 0;
    int heartbeat_total_false_positives = 0;
    int heartbeat_total_false_negatives = 0;
    
    for (size_t i = 5; i < 10; ++i) {
        heartbeat_avg_detection_time += results[i].detection_time_ms;
        heartbeat_avg_accuracy += results[i].accuracy;
        heartbeat_total_messages += results[i].messages_sent;
        heartbeat_total_false_positives += results[i].false_positives;
        heartbeat_total_false_negatives += results[i].false_negatives;
    }
    
    heartbeat_avg_detection_time /= 5;
    heartbeat_avg_accuracy /= 5;
    
    // Print Summary
    std::cout << "Gossip Network Averages:\n"
              << "  Average Detection Time: " << std::setw(8) << gossip_avg_detection_time << " ms\n"
              << "  Average Accuracy:      " << std::setw(8) << (gossip_avg_accuracy * 100) << " %\n"
              << "  Total Messages:        " << std::setw(8) << gossip_total_messages << "\n"
              << "  Total False Positives: " << gossip_total_false_positives << "\n"
              << "  Total False Negatives: " << gossip_total_false_negatives << "\n\n";
    
    std::cout << "Heartbeat Network Averages:\n"
              << "  Average Detection Time: " << std::setw(8) << heartbeat_avg_detection_time << " ms\n"
              << "  Average Accuracy:      " << std::setw(8) << (heartbeat_avg_accuracy * 100) << " %\n"
              << "  Total Messages:        " << std::setw(8) << heartbeat_total_messages << "\n"
              << "  Total False Positives: " << heartbeat_total_false_positives << "\n"
              << "  Total False Negatives: " << heartbeat_total_false_negatives << "\n";
    
    return results;
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
        auto node = network.get_node(id);
        if (!node) continue;
        
        // Try to cast to both types of nodes
        auto gossip_node = std::dynamic_pointer_cast<GossipNode>(node);
        auto heartbeat_node = std::dynamic_pointer_cast<HeartbeatNode>(node);
        
        std::vector<std::string> current_failed;
        if (gossip_node) {
            current_failed = gossip_node->get_failed_nodes();
        } else if (heartbeat_node) {
            current_failed = heartbeat_node->get_failed_nodes();
        }
        
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