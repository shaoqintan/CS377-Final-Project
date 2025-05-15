#include "network.hpp"
#include "node.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>

using namespace std;

Network::Network()
    : rng(random_device{}()),
      loss_dist(0.0, 1.0),
      delay_dist(mean_delay, std_dev_delay) {
    reset_stats();
}

void Network::add_node(const string& node_id, shared_ptr<Node> node) {
    lock_guard<mutex> lock(nodes_mutex);
    nodes[node_id] = node;
}

void Network::remove_node(const string& node_id) {
    lock_guard<mutex> lock(nodes_mutex);
    nodes.erase(node_id);
}

shared_ptr<Node> Network::get_node(const string& node_id) {
    lock_guard<mutex> lock(nodes_mutex);
    auto it = nodes.find(node_id);
    return (it != nodes.end()) ? it->second : nullptr;
}

void Network::send_message(const string& from_id, const string& to_id, const string& content) {
    if (should_drop_message()) {
        update_stats(0, true);
        return;
    }

    int delay = calculate_delay();
    auto delivery_time = chrono::system_clock::now() + chrono::milliseconds(delay);

    Message msg{from_id, to_id, content, delivery_time};
    
    {
        lock_guard<mutex> lock(queue_mutex);
        message_queue.push(msg);
    }

    update_stats(delay, false);
}

void Network::process_messages() {
    auto now = chrono::system_clock::now();
    vector<Message> messages_to_process;

    {
        lock_guard<mutex> lock(queue_mutex);
        while (!message_queue.empty() && message_queue.top().delivery_time <= now) {
            messages_to_process.push_back(message_queue.top());
            message_queue.pop();
        }
    }

    for (const auto& msg : messages_to_process) {
        lock_guard<mutex> lock(nodes_mutex);
        auto it = nodes.find(msg.to_id);
        if (it != nodes.end()) {
            it->second->receive_message(msg.from_id, msg.content);
        }
    }
}

void Network::simulate_network_partition(const vector<string>& partition1,
                                      const vector<string>& partition2,
                                      int duration_ms) {
    lock_guard<mutex> lock(partition_mutex);
    
    unordered_set<string> set1(partition1.begin(), partition1.end());
    unordered_set<string> set2(partition2.begin(), partition2.end());
    
    partitions[partition1[0]] = set1;
    partitions[partition2[0]] = set2;
}

void Network::heal_network_partition() {
    lock_guard<mutex> lock(partition_mutex);
    partitions.clear();
}

Network::NetworkStats Network::get_stats() const {
    NetworkStats current_stats;
    current_stats.delivered_messages = stats.delivered_messages.load(memory_order_relaxed);
    current_stats.dropped_messages = stats.dropped_messages.load(memory_order_relaxed);
    current_stats.total_delay = stats.total_delay.load(memory_order_relaxed);
    return current_stats;
}

void Network::reset_stats() {
    stats.delivered_messages.store(0, memory_order_relaxed);
    stats.dropped_messages.store(0, memory_order_relaxed);
    stats.total_delay.store(0.0, memory_order_relaxed);
}

bool Network::should_drop_message() {
    return loss_dist(rng) < message_loss_rate;
}

int Network::calculate_delay() {
    return max(0, static_cast<int>(delay_dist(rng)));
}

void Network::update_stats(int delay, bool dropped) {
    if (dropped) {
        stats.dropped_messages.fetch_add(1, memory_order_relaxed);
    } else {
        stats.delivered_messages.fetch_add(1, memory_order_relaxed);
        double current_delay = stats.total_delay.load(memory_order_relaxed);
        stats.total_delay.store(current_delay + delay, memory_order_relaxed);
    }
} 