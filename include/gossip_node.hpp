#pragma once

#include "node.hpp"
#include <unordered_map>
#include <random>

using namespace std;

class GossipNode : public Node {
private:
    // Node state tracking
    struct NodeState {
        bool is_alive;
        chrono::system_clock::time_point last_seen;
        int suspicion_level;
    };
    unordered_map<string, NodeState> node_states;
    mutable mutex states_mutex;

    // Gossip parameters
    const int gossip_interval_ms = 1000;  // Time between gossip rounds
    const int suspicion_threshold = 3;    // Number of missed rounds before marking as failed
    const int fanout = 3;                 // Number of peers to gossip with each round
    
    // Random number generation for peer selection
    mt19937 rng;
    uniform_int_distribution<int> peer_dist;

    // Metrics
    struct Metrics {
        int messages_sent;
        int messages_received;
        int false_positives;
        int false_negatives;
        chrono::system_clock::time_point last_metrics_reset;
    } metrics;

public:
    GossipNode(const string& node_id, const vector<string>& peer_ids);
    ~GossipNode() override = default;

    // Core functionality
    void start() override;
    void send_message(const string& to_id, const string& content) override;
    void process_message(const Message& msg) override;

    // State management
    vector<string> get_failed_nodes() const;
    void add_peer(const string& peer_id);
    void remove_peer(const string& peer_id);

    // Metrics
    Metrics get_metrics() const;
    void reset_metrics();

protected:
    void periodic_task() override;

private:
    // Helper functions
    void gossip_round();
    vector<string> select_random_peers();
    void update_node_state(const string& node_id, bool is_alive);
    bool is_node_failed(const string& node_id) const;
    void serialize_state(string& out) const;
    void deserialize_state(const string& in);
}; 