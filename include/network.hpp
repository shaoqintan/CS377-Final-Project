#pragma once

#include "node.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

class Network {
private:
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::mutex nodes_mutex;
    
    // Network parameters
    const int message_delay_ms = 50;  // Simulated network delay
    const double message_loss_rate = 0.01;  // 1% message loss rate
    
    // Random number generation for message loss simulation
    std::mt19937 rng;
    std::uniform_real_distribution<double> loss_dist;

public:
    Network();
    ~Network() = default;

    // Node management
    void add_node(const std::string& node_id, std::shared_ptr<Node> node);
    void remove_node(const std::string& node_id);
    std::shared_ptr<Node> get_node(const std::string& node_id);
    
    // Message routing
    void send_message(const std::string& from_id, const std::string& to_id, const std::string& content);
    
    // Network simulation
    void simulate_network_partition(const std::vector<std::string>& partition1, 
                                  const std::vector<std::string>& partition2,
                                  int duration_ms);
    void simulate_node_failure(const std::string& node_id);
    void simulate_node_recovery(const std::string& node_id);
    
    // Network statistics
    struct NetworkStats {
        int total_messages;
        int lost_messages;
        int delayed_messages;
        double average_delay_ms;
    };
    NetworkStats get_stats() const;
    void reset_stats();

private:
    // Helper functions
    bool should_drop_message();
    int calculate_delay();
    void update_stats(int delay, bool lost);
    
    // Statistics
    struct {
        std::atomic<int> total_messages{0};
        std::atomic<int> lost_messages{0};
        std::atomic<int> delayed_messages{0};
        std::atomic<double> total_delay{0.0};
    } stats;
}; 