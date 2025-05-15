#pragma once

#include "node.hpp"
#include <unordered_map>
#include <chrono>

using namespace std;

class HeartbeatNode : public Node {
private:
    // Node state tracking
    struct NodeState {
        bool is_alive;
        chrono::system_clock::time_point last_heartbeat;
    };
    unordered_map<string, NodeState> node_states;
    mutable mutex states_mutex;

    // Heartbeat parameters
    const int heartbeat_interval_ms = 1000;    // Time between heartbeats
    const int failure_threshold_ms = 3000;     // Time without heartbeat before marking as failed
    bool is_master;                            // Whether this node is the master node

    // Metrics
    struct Metrics {
        int heartbeats_sent;
        int heartbeats_received;
        int false_positives;
        int false_negatives;
        chrono::system_clock::time_point last_metrics_reset;
    } metrics;

public:
    HeartbeatNode(const string& node_id, bool is_master_node);
    ~HeartbeatNode() override = default;

    // Core functionality
    void start() override;
    void send_message(const string& to_id, const string& content) override;
    void process_message(const Message& msg) override;

    // State management
    vector<string> get_failed_nodes() const;
    void add_node(const string& node_id);
    void remove_node(const string& node_id);
    bool is_master_node() const { return is_master; }

    // Metrics
    Metrics get_metrics() const;
    void reset_metrics();

protected:
    void periodic_task() override;

private:
    // Helper functions
    void send_heartbeat();
    void check_node_health();
    void update_node_state(const string& node_id, bool is_alive);
    bool is_node_failed(const string& node_id) const;
}; 