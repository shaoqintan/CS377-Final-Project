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
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate node failure
    simulate_failures({failed_node});
    
    // Wait for convergence with a reasonable timeout
    wait_for_convergence({failed_node}, 2 * 3000);  // 2x the typical detection time
    
    // Calculate actual detection time
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("Single Node Failure Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

Simulator::TestResult Simulator::run_multiple_failures_test(int num_nodes, int num_failures) {
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
    
    // Choose random nodes to fail
    std::vector<std::string> failed_nodes;
    for (int i = 0; i < num_failures; ++i) {
        failed_nodes.push_back("node" + std::to_string(i));
    }
    std::random_shuffle(failed_nodes.begin(), failed_nodes.end());
    
    // Reset network stats before failure simulation
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate failures
    simulate_failures(failed_nodes);
    
    // Wait for convergence with a reasonable timeout
    wait_for_convergence(failed_nodes, 2 * 3000);  // 2x the typical detection time
    
    // Calculate actual detection time
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("Multiple Failures Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

Simulator::TestResult Simulator::run_network_partition_test(int num_nodes) {
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
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate network partition
    network.simulate_network_partition(partition1, partition2, 3000);
    
    // Wait for convergence with a reasonable timeout
    // In this case, we want both partitions to detect each other as failed
    std::vector<std::string> all_nodes;
    all_nodes.insert(all_nodes.end(), partition1.begin(), partition1.end());
    all_nodes.insert(all_nodes.end(), partition2.begin(), partition2.end());
    wait_for_convergence(all_nodes, 2 * 3000);  // 2x the typical detection time
    
    // Calculate actual detection time
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);
    
    // Heal partition
    network.heal_network_partition();
    
    auto result = collect_metrics("Network Partition Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

Simulator::TestResult Simulator::run_high_load_test(int num_nodes) {
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
    
    // Reset network stats before high load test
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::high_resolution_clock::now();
    
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
    
    // Wait for all messages to be delivered
    // In this case, we want to ensure no nodes are marked as failed
    std::vector<std::string> empty_failed;  // Empty list means no nodes should be marked as failed
    wait_for_convergence(empty_failed, 2 * 3000);  // 2x the typical detection time
    
    // Calculate delivery time
    auto delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("High Load Test");
    result.detection_time_ms = delivery_time.count();
    return result;
}

Simulator::TestResult Simulator::run_recovery_test(int num_nodes) {
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
    
    // Choose a random node to fail and recover
    std::string node_id = "node" + std::to_string(rand() % num_nodes);
    
    // Reset network stats before recovery test
    network.reset_stats();
    
    // Record start time
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate failure
    simulate_failures({node_id});
    
    // Wait for failure detection
    wait_for_convergence({node_id}, 2 * 3000);  // 2x the typical detection time
    
    // Simulate recovery
    simulate_recoveries({node_id});
    
    // Wait for recovery detection
    // In this case, we want all nodes to agree that the recovered node is no longer failed
    std::vector<std::string> empty_failed;  // Empty list means no nodes should be marked as failed
    wait_for_convergence(empty_failed, 2 * 3000);  // 2x the typical detection time
    
    // Calculate total detection time (failure + recovery)
    auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("Recovery Test");
    result.detection_time_ms = detection_time.count();
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

void Simulator::wait_for_convergence(const std::vector<std::string>& must_fail, int timeout_ms) {
    auto start = std::chrono::high_resolution_clock::now();
    while (!check_convergence(must_fail)) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - start).count() > timeout_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        network.process_messages();
    }
}

bool Simulator::check_convergence(const std::vector<std::string>& must_be_failed) {
    for (auto& [id, node] : network.get_nodes()) {
        if (!node) continue;
        auto failed = node->get_failed_nodes();  // common API in both nodes
        for (auto& victim : must_be_failed) {
            if (std::find(failed.begin(), failed.end(), victim) == failed.end()) {
                return false;  // someone hasn't noticed yet
            }
        }
    }
    return true;  // global agreement reached
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
    
    // Count false positives and negatives by comparing each node's view
    result.false_positives = 0;
    result.false_negatives = 0;
    int true_positives = 0;
    
    // Get all nodes' views of failed nodes
    std::unordered_map<std::string, std::vector<std::string>> node_views;
    for (auto& [id, node] : network.get_nodes()) {
        if (!node) continue;
        node_views[id] = node->get_failed_nodes();
    }
    
    // Compare each node's view with others
    for (auto& [id1, view1] : node_views) {
        for (auto& [id2, view2] : node_views) {
            if (id1 == id2) continue;
            
            // Check for false positives (nodes marked as failed but are alive)
            for (const auto& failed : view1) {
                if (std::find(view2.begin(), view2.end(), failed) == view2.end()) {
                    result.false_positives++;
                } else {
                    true_positives++;
                }
            }
            
            // Check for false negatives (nodes not marked as failed but are dead)
            for (const auto& failed : view2) {
                if (std::find(view1.begin(), view1.end(), failed) == view1.end()) {
                    result.false_negatives++;
                }
            }
        }
    }
    
    // Calculate accuracy
    result.accuracy = calculate_accuracy(true_positives, result.false_positives, result.false_negatives);
    
    return result;
}

double Simulator::calculate_accuracy(int true_positives, int false_positives, int false_negatives) {
    if (true_positives + false_positives + false_negatives == 0) return 1.0;
    return static_cast<double>(true_positives) / (true_positives + false_positives + false_negatives);
} 