#include "network.hpp"
#include <thread>
#include <chrono>
#include <random>

Network::Network() 
    : rng(std::random_device{}()), loss_dist(0.0, 1.0) {}

void Network::add_node(const std::string& node_id, std::shared_ptr<Node> node) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    nodes[node_id] = node;
}

void Network::remove_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    nodes.erase(node_id);
}

std::shared_ptr<Node> Network::get_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    auto it = nodes.find(node_id);
    return (it != nodes.end()) ? it->second : nullptr;
}

void Network::send_message(const std::string& from_id, const std::string& to_id, const std::string& content) {
    // Check if message should be dropped
    if (should_drop_message()) {
        update_stats(0, true);
        return;
    }

    // Calculate message delay
    int delay = calculate_delay();
    
    // Create a thread to simulate message delay
    std::thread([this, from_id, to_id, content, delay]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        
        auto target_node = get_node(to_id);
        if (target_node) {
            target_node->receive_message(from_id, content);
        }
        
        update_stats(delay, false);
    }).detach();
}

void Network::simulate_network_partition(const std::vector<std::string>& partition1, 
                                       const std::vector<std::string>& partition2,
                                       int duration_ms) {
    // Create sets for faster lookup
    std::unordered_set<std::string> set1(partition1.begin(), partition1.end());
    std::unordered_set<std::string> set2(partition2.begin(), partition2.end());
    
    // Store original message loss rate
    const double original_loss_rate = message_loss_rate;
    
    // Temporarily increase message loss rate between partitions
    const_cast<double&>(message_loss_rate) = 1.0;  // 100% message loss between partitions
    
    // Wait for the specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    
    // Restore original message loss rate
    const_cast<double&>(message_loss_rate) = original_loss_rate;
}

void Network::simulate_node_failure(const std::string& node_id) {
    auto node = get_node(node_id);
    if (node) {
        node->set_alive(false);
    }
}

void Network::simulate_node_recovery(const std::string& node_id) {
    auto node = get_node(node_id);
    if (node) {
        node->set_alive(true);
    }
}

Network::NetworkStats Network::get_stats() const {
    NetworkStats stats;
    stats.total_messages = this->stats.total_messages;
    stats.lost_messages = this->stats.lost_messages;
    stats.delayed_messages = this->stats.delayed_messages;
    stats.average_delay_ms = (this->stats.total_messages > 0) 
        ? this->stats.total_delay / this->stats.total_messages 
        : 0.0;
    return stats;
}

void Network::reset_stats() {
    stats.total_messages = 0;
    stats.lost_messages = 0;
    stats.delayed_messages = 0;
    stats.total_delay = 0.0;
}

bool Network::should_drop_message() {
    return loss_dist(rng) < message_loss_rate;
}

int Network::calculate_delay() {
    // Add some random variation to the base delay
    std::normal_distribution<double> delay_dist(message_delay_ms, message_delay_ms * 0.2);
    return std::max(0, static_cast<int>(delay_dist(rng)));
}

void Network::update_stats(int delay, bool lost) {
    stats.total_messages++;
    if (lost) {
        stats.lost_messages++;
    }
    if (delay > 0) {
        stats.delayed_messages++;
        stats.total_delay += delay;
    }
} 