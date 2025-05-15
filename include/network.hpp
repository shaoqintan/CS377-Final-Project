#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <random>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <memory>

using namespace std;

class Node;  // Forward declaration

class Network {
private:
    struct Message {
        string from_id;
        string to_id;
        string content;
        chrono::system_clock::time_point delivery_time;

        bool operator<(const Message& other) const {
            return delivery_time > other.delivery_time;  // For min-heap priority queue
        }
    };

    mt19937 rng;
    uniform_real_distribution<double> loss_dist;
    normal_distribution<double> delay_dist;

    unordered_map<string, shared_ptr<Node>> nodes;
    mutex nodes_mutex;
    
    priority_queue<Message> message_queue;
    mutex queue_mutex;

    // Network parameters
    const double message_loss_rate = 0.1;  // 10% message loss rate
    const double mean_delay = 50.0;        // Mean delay in milliseconds
    const double std_dev_delay = 10.0;     // Standard deviation of delay

    // Network partition simulation
    unordered_map<string, unordered_set<string>> partitions;
    mutex partition_mutex;

    struct NetworkStats {
        atomic<int> delivered_messages;
        atomic<int> dropped_messages;
        atomic<double> total_delay;

        NetworkStats() : delivered_messages(0), dropped_messages(0), total_delay(0.0) {}
        
        // Custom copy constructor
        NetworkStats(const NetworkStats& other) 
            : delivered_messages(other.delivered_messages.load())
            , dropped_messages(other.dropped_messages.load())
            , total_delay(other.total_delay.load()) {}
    } stats;

    bool should_drop_message();
    int calculate_delay();
    void update_stats(int delay, bool dropped);

public:
    Network();
    ~Network() = default;

    void add_node(const string& node_id, shared_ptr<Node> node);
    void remove_node(const string& node_id);
    shared_ptr<Node> get_node(const string& node_id);
    void send_message(const string& from_id, const string& to_id, const string& content);
    void process_messages();
    void simulate_network_partition(const vector<string>& partition1,
                                  const vector<string>& partition2,
                                  int duration_ms);
    void heal_network_partition();
    NetworkStats get_stats() const;
    void reset_stats();
    const unordered_map<string, shared_ptr<Node>>& get_nodes() const {
        return nodes;  // guarded access is OK – we only read
    }
}; 