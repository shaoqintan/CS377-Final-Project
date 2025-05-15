#include "simulator.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>
#include <unordered_set>
#include <iomanip>

using namespace std;

Simulator::Simulator() {}

void Simulator::setup_gossip_network(int num_nodes) {
    cleanup_network();
    
    vector<string> node_ids;
    for (int i = 0; i < num_nodes; ++i) {
        node_ids.push_back("node" + to_string(i));
    }
    
    for (const auto& id : node_ids) {
        auto node = make_shared<GossipNode>(id, node_ids);
        network.add_node(id, node);
        node->start();
    }
}

void Simulator::setup_heartbeat_network(int num_nodes) {
    cleanup_network();
    
    vector<string> node_ids;
    for (int i = 0; i < num_nodes; ++i) {
        node_ids.push_back("node" + to_string(i));
    }
    
    for (const auto& id : node_ids) {
        auto node = make_shared<HeartbeatNode>(id, id == "node0");
        network.add_node(id, node);
        node->start();
    }
}

void Simulator::cleanup_network() {
    for (int i = 0; i < 100; ++i) {
        string id = "node" + to_string(i);
        auto node = network.get_node(id);
        if (node) {
            node->stop();
        }
    }
    
    for (int i = 0; i < 100; ++i) {
        string id = "node" + to_string(i);
        network.remove_node(id);
    }
}

Simulator::TestResult Simulator::run_single_node_failure_test(int num_nodes) {
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + to_string(i),
                                   "node" + to_string(j),
                                   "initial_traffic");
            }
        }
    }
    
    for (int i = 0; i < 50; ++i) {
        network.process_messages();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    network.reset_stats();
    
    string failed_node = "node" + to_string(rand() % num_nodes);
    
    auto start_time = chrono::high_resolution_clock::now();
    
    simulate_failures({failed_node});
    
    wait_for_convergence({failed_node}, 2 * 3000);
    
    auto detection_time = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("Single Node Failure Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

Simulator::TestResult Simulator::run_multiple_failures_test(int num_nodes, int num_failures) {
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + to_string(i),
                                   "node" + to_string(j),
                                   "initial_traffic");
            }
        }
    }
    
    for (int i = 0; i < 50; ++i) {
        network.process_messages();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    vector<string> failed_nodes;
    for (int i = 0; i < num_failures; ++i) {
        failed_nodes.push_back("node" + to_string(i));
    }
    random_shuffle(failed_nodes.begin(), failed_nodes.end());
    
    network.reset_stats();
    
    auto start_time = chrono::high_resolution_clock::now();
    
    simulate_failures(failed_nodes);
    
    wait_for_convergence(failed_nodes, 2 * 3000);
    
    auto detection_time = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("Multiple Failures Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

Simulator::TestResult Simulator::run_network_partition_test(int num_nodes) {
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + to_string(i),
                                   "node" + to_string(j),
                                   "initial_traffic");
            }
        }
    }
    
    for (int i = 0; i < 50; ++i) {
        network.process_messages();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    vector<string> partition1, partition2;
    for (int i = 0; i < num_nodes; ++i) {
        if (i < num_nodes / 2) {
            partition1.push_back("node" + to_string(i));
        } else {
            partition2.push_back("node" + to_string(i));
        }
    }
    
    network.reset_stats();
    
    auto start_time = chrono::high_resolution_clock::now();
    
    network.simulate_network_partition(partition1, partition2, 3000);
    
    vector<string> all_nodes;
    all_nodes.insert(all_nodes.end(), partition1.begin(), partition1.end());
    all_nodes.insert(all_nodes.end(), partition2.begin(), partition2.end());
    wait_for_convergence(all_nodes, 2 * 3000);
    
    auto detection_time = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start_time);
    
    network.heal_network_partition();
    
    auto result = collect_metrics("Network Partition Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

Simulator::TestResult Simulator::run_high_load_test(int num_nodes) {
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + to_string(i),
                                   "node" + to_string(j),
                                   "initial_traffic");
            }
        }
    }
    
    for (int i = 0; i < 50; ++i) {
        network.process_messages();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    network.reset_stats();
    
    auto start_time = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + to_string(i),
                                   "node" + to_string(j),
                                   "high_load_test");
            }
        }
    }
    
    vector<string> empty_failed;
    wait_for_convergence(empty_failed, 2 * 3000);
    
    auto delivery_time = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("High Load Test");
    result.detection_time_ms = delivery_time.count();
    return result;
}

Simulator::TestResult Simulator::run_recovery_test(int num_nodes) {
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i != j) {
                network.send_message("node" + to_string(i),
                                   "node" + to_string(j),
                                   "initial_traffic");
            }
        }
    }
    
    for (int i = 0; i < 50; ++i) {
        network.process_messages();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    string node_id = "node" + to_string(rand() % num_nodes);
    
    network.reset_stats();
    
    auto start_time = chrono::high_resolution_clock::now();
    
    simulate_failures({node_id});
    
    wait_for_convergence({node_id}, 2 * 3000);
    
    simulate_recoveries({node_id});
    
    vector<string> empty_failed;
    wait_for_convergence(empty_failed, 2 * 3000);
    
    auto detection_time = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start_time);
    
    auto result = collect_metrics("Recovery Test");
    result.detection_time_ms = detection_time.count();
    return result;
}

vector<Simulator::TestResult> Simulator::compare_algorithms(int num_nodes, bool force_rerun) {
    vector<TestResult> results;
    string cache_key = "gossip_" + to_string(num_nodes);
    
    if (!force_rerun && cached_results.find(cache_key) != cached_results.end()) {
        results = cached_results[cache_key];
    } else {
        cout << "\nRunning Gossip Network Tests...\n";
        setup_gossip_network(num_nodes);
        vector<TestResult> gossip_results;
        gossip_results.push_back(run_single_node_failure_test(num_nodes));
        gossip_results.push_back(run_multiple_failures_test(num_nodes, num_nodes / 2));
        gossip_results.push_back(run_network_partition_test(num_nodes));
        gossip_results.push_back(run_high_load_test(num_nodes));
        gossip_results.push_back(run_recovery_test(num_nodes));
        cleanup_network();
        
        cached_results[cache_key] = gossip_results;
        results = gossip_results;
    }
    
    cache_key = "heartbeat_" + to_string(num_nodes);
    vector<TestResult> heartbeat_results;
    
    if (!force_rerun && cached_results.find(cache_key) != cached_results.end()) {
        heartbeat_results = cached_results[cache_key];
    } else {
        cout << "\nRunning Heartbeat Network Tests...\n";
        setup_heartbeat_network(num_nodes);
        heartbeat_results.push_back(run_single_node_failure_test(num_nodes));
        heartbeat_results.push_back(run_multiple_failures_test(num_nodes, num_nodes / 2));
        heartbeat_results.push_back(run_network_partition_test(num_nodes));
        heartbeat_results.push_back(run_high_load_test(num_nodes));
        heartbeat_results.push_back(run_recovery_test(num_nodes));
        cleanup_network();
        
        cached_results[cache_key] = heartbeat_results;
    }
    
    results.insert(results.end(), heartbeat_results.begin(), heartbeat_results.end());
    
    cout << "\nAlgorithm Comparison Results:\n";
    cout << "===========================\n";
    
    cout << "\nGossip Network Results:\n";
    cout << "---------------------------\n";
    for (size_t i = 0; i < 5; ++i) {
        const auto& result = results[i];
        cout << "Test: " << result.test_name << "\n"
              << "  Detection Time: " << setw(8) << result.detection_time_ms << " ms\n"
              << "  Accuracy:      " << setw(8) << (result.accuracy * 100) << " %\n"
              << "  Messages Sent: " << setw(8) << result.messages_sent << "\n"
              << "  False Positives: " << result.false_positives << "\n"
              << "  False Negatives: " << result.false_negatives << "\n\n";
    }
    
    cout << "\nHeartbeat Network Results:\n";
    cout << "---------------------------\n";
    for (size_t i = 5; i < 10; ++i) {
        const auto& result = results[i];
        cout << "Test: " << result.test_name << "\n"
                  << "  Detection Time: " << setw(8) << result.detection_time_ms << " ms\n"
                  << "  Accuracy:      " << setw(8) << (result.accuracy * 100) << " %\n"
                  << "  Messages Sent: " << setw(8) << result.messages_sent << "\n"
                  << "  False Positives: " << result.false_positives << "\n"
                  << "  False Negatives: " << result.false_negatives << "\n\n";
    }
    
    cout << "\nSummary Statistics:\n";
    cout << "---------------------------\n";
    
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
    
    cout << "Gossip Network Averages:\n"
              << "  Average Detection Time: " << setw(8) << gossip_avg_detection_time << " ms\n"
              << "  Average Accuracy:      " << setw(8) << (gossip_avg_accuracy * 100) << " %\n"
              << "  Total Messages:        " << setw(8) << gossip_total_messages << "\n"
              << "  Total False Positives: " << gossip_total_false_positives << "\n"
              << "  Total False Negatives: " << gossip_total_false_negatives << "\n\n";
    
    cout << "Heartbeat Network Averages:\n"
              << "  Average Detection Time: " << setw(8) << heartbeat_avg_detection_time << " ms\n"
              << "  Average Accuracy:      " << setw(8) << (heartbeat_avg_accuracy * 100) << " %\n"
              << "  Total Messages:        " << setw(8) << heartbeat_total_messages << "\n"
              << "  Total False Positives: " << heartbeat_total_false_positives << "\n"
              << "  Total False Negatives: " << heartbeat_total_false_negatives << "\n";
    
    return results;
}

void Simulator::wait_for_convergence(const vector<string>& must_fail, int timeout_ms) {
    auto start = chrono::high_resolution_clock::now();
    while (!check_convergence(must_fail)) {
        if (chrono::duration_cast<chrono::milliseconds>(
               chrono::high_resolution_clock::now() - start).count() > timeout_ms) {
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(50));
        network.process_messages();
    }
}

bool Simulator::check_convergence(const vector<string>& must_be_failed) {
    for (auto& [id, node] : network.get_nodes()) {
        if (!node) continue;
        auto failed = node->get_failed_nodes();
        for (auto& victim : must_be_failed) {
            if (find(failed.begin(), failed.end(), victim) == failed.end()) {
                return false;
            }
        }
    }
    return true;
}

void Simulator::simulate_failures(const vector<string>& node_ids) {
    for (const auto& node_id : node_ids) {
        auto node = network.get_node(node_id);
        if (node) {
            node->set_alive(false);
        }
    }
}

void Simulator::simulate_recoveries(const vector<string>& node_ids) {
    for (const auto& node_id : node_ids) {
        auto node = network.get_node(node_id);
        if (node) {
            node->set_alive(true);
        }
    }
}

Simulator::TestResult Simulator::collect_metrics(const string& test_name) {
    TestResult result;
    result.test_name = test_name;
    
    auto net_stats = network.get_stats();
    result.messages_sent = net_stats.delivered_messages + net_stats.dropped_messages;
    
    result.false_positives = 0;
    result.false_negatives = 0;
    int true_positives = 0;
    
    unordered_map<string, vector<string>> node_views;
    for (auto& [id, node] : network.get_nodes()) {
        if (!node) continue;
        node_views[id] = node->get_failed_nodes();
    }
    
    for (auto& [id1, view1] : node_views) {
        for (auto& [id2, view2] : node_views) {
            if (id1 == id2) continue;
            
            for (const auto& failed : view1) {
                if (find(view2.begin(), view2.end(), failed) == view2.end()) {
                    result.false_positives++;
                } else {
                    true_positives++;
                }
            }
            
            for (const auto& failed : view2) {
                if (find(view1.begin(), view1.end(), failed) == view1.end()) {
                    result.false_negatives++;
                }
            }
        }
    }
    
    result.accuracy = calculate_accuracy(true_positives, result.false_positives, result.false_negatives);
    
    return result;
}

double Simulator::calculate_accuracy(int true_positives, int false_positives, int false_negatives) {
    if (true_positives + false_positives + false_negatives == 0) return 1.0;
    return static_cast<double>(true_positives) / (true_positives + false_positives + false_negatives);
} 