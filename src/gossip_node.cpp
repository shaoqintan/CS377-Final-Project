#include "gossip_node.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>

GossipNode::GossipNode(const std::string& node_id, const std::vector<std::string>& peer_ids)
    : Node(node_id), rng(std::random_device{}()) {
    
    // Initialize node states
    for (const auto& peer_id : peer_ids) {
        node_states[peer_id] = {true, get_current_time(), 0};
    }
    node_states[node_id] = {true, get_current_time(), 0};  // Add self
    
    // Initialize metrics
    metrics = {0, 0, 0, 0, get_current_time()};
}

void GossipNode::start() {
    is_running = true;
    node_thread = std::thread(&GossipNode::run, this);
}

void GossipNode::send_message(const std::string& to_id, const std::string& content) {
    // In a real implementation, this would send over the network
    // For simulation, we'll just increment the message counter
    metrics.messages_sent++;
}

void GossipNode::process_message(const Message& msg) {
    metrics.messages_received++;
    
    // Update sender's state
    {
        std::lock_guard<std::mutex> lock(states_mutex);
        auto it = node_states.find(msg.from_id);
        if (it != node_states.end()) {
            it->second.last_seen = msg.timestamp;
            it->second.suspicion_level = 0;
            it->second.is_alive = true;  // Reset alive status when we hear from a node
        }
    }
    
    // Process the gossip state
    deserialize_state(msg.content);
}

void GossipNode::periodic_task() {
    static auto last_gossip = get_current_time();
    auto now = get_current_time();
    
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_gossip).count() >= gossip_interval_ms) {
        gossip_round();
        last_gossip = now;
        
        // Update suspicion levels
        std::lock_guard<std::mutex> lock(states_mutex);
        for (auto& [id, state] : node_states) {
            if (id != this->id) {  // Don't check self
                auto time_since_last_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
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
    std::string state_str;
    serialize_state(state_str);
    
    for (const auto& peer : peers) {
        send_message(peer, state_str);
    }
}

std::vector<std::string> GossipNode::select_random_peers() {
    std::vector<std::string> peers;
    std::lock_guard<std::mutex> lock(states_mutex);
    
    // Get all peer IDs
    for (const auto& [id, state] : node_states) {
        if (id != this->id) {  // Don't include self
            peers.push_back(id);
        }
    }
    
    // Randomly select fanout number of peers
    if (peers.size() <= fanout) {
        return peers;
    }
    
    std::shuffle(peers.begin(), peers.end(), rng);
    return std::vector<std::string>(peers.begin(), peers.begin() + fanout);
}

void GossipNode::update_node_state(const std::string& node_id, bool is_alive) {
    std::lock_guard<std::mutex> lock(states_mutex);
    auto it = node_states.find(node_id);
    if (it != node_states.end()) {
        it->second.is_alive = is_alive;
        it->second.last_seen = get_current_time();
        it->second.suspicion_level = 0;
    }
}

std::vector<std::string> GossipNode::get_failed_nodes() const {
    std::vector<std::string> failed;
    std::lock_guard<std::mutex> lock(states_mutex);
    
    for (const auto& [id, state] : node_states) {
        if (!state.is_alive || state.suspicion_level >= suspicion_threshold) {
            failed.push_back(id);
        }
    }
    
    return failed;
}

void GossipNode::serialize_state(std::string& out) const {
    std::stringstream ss;
    std::lock_guard<std::mutex> lock(states_mutex);
    
    for (const auto& [id, state] : node_states) {
        ss << id << ":" << state.is_alive << ":" 
           << std::chrono::system_clock::to_time_t(state.last_seen) << ";";
    }
    
    out = ss.str();
}

void GossipNode::deserialize_state(const std::string& in) {
    std::stringstream ss(in);
    std::string entry;
    
    while (std::getline(ss, entry, ';')) {
        std::stringstream entry_ss(entry);
        std::string id, is_alive_str, timestamp_str;
        
        if (std::getline(entry_ss, id, ':') &&
            std::getline(entry_ss, is_alive_str, ':') &&
            std::getline(entry_ss, timestamp_str, ':')) {
            
            bool is_alive = (is_alive_str == "1");
            time_t timestamp = std::stoll(timestamp_str);
            
            std::lock_guard<std::mutex> lock(states_mutex);
            auto it = node_states.find(id);
            if (it != node_states.end()) {
                it->second.is_alive = is_alive;
                it->second.last_seen = std::chrono::system_clock::from_time_t(timestamp);
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

void GossipNode::add_peer(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(states_mutex);
    node_states[peer_id] = {true, get_current_time(), 0};
}

void GossipNode::remove_peer(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(states_mutex);
    node_states.erase(peer_id);
} 