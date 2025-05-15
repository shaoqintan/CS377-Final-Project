#include "gossip_node.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>

using namespace std;

GossipNode::GossipNode(const string& node_id, const vector<string>& peer_ids)
    : Node(node_id), rng(random_device{}()) {
    for (const auto& peer_id : peer_ids) {
        node_states[peer_id] = {true, get_current_time(), 0};
    }
    node_states[node_id] = {true, get_current_time(), 0};
    metrics = {0, 0, 0, 0, get_current_time()};
}

void GossipNode::start() {
    is_running = true;
    node_thread = thread(&GossipNode::run, this);
}

void GossipNode::send_message(const string& to_id, const string& content) {
    metrics.messages_sent++;
}

void GossipNode::process_message(const Message& msg) {
    metrics.messages_received++;
    
    {
        lock_guard<mutex> lock(states_mutex);
        auto it = node_states.find(msg.from_id);
        if (it != node_states.end()) {
            it->second.last_seen = msg.timestamp;
            it->second.suspicion_level = 0;
            it->second.is_alive = true;
        }
    }
    
    deserialize_state(msg.content);
}

void GossipNode::periodic_task() {
    static auto last_gossip = get_current_time();
    auto now = get_current_time();
    
    if (chrono::duration_cast<chrono::milliseconds>(now - last_gossip).count() >= gossip_interval_ms) {
        gossip_round();
        last_gossip = now;
        
        lock_guard<mutex> lock(states_mutex);
        for (auto& [id, state] : node_states) {
            if (id != this->id) {
                auto time_since_last_seen = chrono::duration_cast<chrono::milliseconds>(
                    now - state.last_seen).count();
                
                if (time_since_last_seen > gossip_interval_ms) {
                    state.suspicion_level++;
                    if (state.suspicion_level >= suspicion_threshold) {
                        state.is_alive = false;
                    }
                }
            }
        }
    }
}

void GossipNode::gossip_round() {
    auto peers = select_random_peers();
    string state_str;
    serialize_state(state_str);
    
    for (const auto& peer : peers) {
        send_message(peer, state_str);
    }
}

vector<string> GossipNode::select_random_peers() {
    vector<string> peers;
    lock_guard<mutex> lock(states_mutex);
    
    for (const auto& [id, state] : node_states) {
        if (id != this->id) {
            peers.push_back(id);
        }
    }
    
    if (peers.size() <= fanout) {
        return peers;
    }
    
    shuffle(peers.begin(), peers.end(), rng);
    return vector<string>(peers.begin(), peers.begin() + fanout);
}

void GossipNode::update_node_state(const string& node_id, bool is_alive) {
    lock_guard<mutex> lock(states_mutex);
    auto it = node_states.find(node_id);
    if (it != node_states.end()) {
        it->second.is_alive = is_alive;
        it->second.last_seen = get_current_time();
        it->second.suspicion_level = 0;
    }
}

vector<string> GossipNode::get_failed_nodes() const {
    vector<string> failed;
    lock_guard<mutex> lock(states_mutex);
    
    for (const auto& [id, state] : node_states) {
        if (!state.is_alive || state.suspicion_level >= suspicion_threshold) {
            failed.push_back(id);
        }
    }
    
    return failed;
}

void GossipNode::serialize_state(string& out) const {
    stringstream ss;
    lock_guard<mutex> lock(states_mutex);
    
    for (const auto& [id, state] : node_states) {
        ss << id << ":" << state.is_alive << ":" 
           << chrono::system_clock::to_time_t(state.last_seen) << ";";
    }
    
    out = ss.str();
}

void GossipNode::deserialize_state(const string& in) {
    stringstream ss(in);
    string entry;
    
    while (getline(ss, entry, ';')) {
        stringstream entry_ss(entry);
        string id, is_alive_str, timestamp_str;
        
        if (getline(entry_ss, id, ':') &&
            getline(entry_ss, is_alive_str, ':') &&
            getline(entry_ss, timestamp_str, ':')) {
            
            bool is_alive = (is_alive_str == "1");
            time_t timestamp = stoll(timestamp_str);
            
            lock_guard<mutex> lock(states_mutex);
            auto it = node_states.find(id);
            if (it != node_states.end()) {
                it->second.is_alive = is_alive;
                it->second.last_seen = chrono::system_clock::from_time_t(timestamp);
                it->second.suspicion_level = 0;
            }
        }
    }
}

GossipNode::Metrics GossipNode::get_metrics() const {
    return metrics;
}

void GossipNode::reset_metrics() {
    metrics = {0, 0, 0, 0, get_current_time()};
}

void GossipNode::add_peer(const string& peer_id) {
    lock_guard<mutex> lock(states_mutex);
    node_states[peer_id] = {true, get_current_time(), 0};
}

void GossipNode::remove_peer(const string& peer_id) {
    lock_guard<mutex> lock(states_mutex);
    node_states.erase(peer_id);
} 