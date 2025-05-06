#include "heartbeat_node.hpp"
#include <sstream>

HeartbeatNode::HeartbeatNode(const std::string& node_id, bool is_master_node)
    : Node(node_id), is_master(is_master_node) {
    
    // Initialize metrics
    metrics = {0, 0, 0, 0, get_current_time()};
    
    // Initialize self state
    node_states[node_id] = {true, get_current_time()};
}

void HeartbeatNode::start() {
    is_running = true;
    node_thread = std::thread(&HeartbeatNode::run, this);
}

void HeartbeatNode::send_message(const std::string& to_id, const std::string& content) {
    // In a real implementation, this would send over the network
    // For simulation, we'll just increment the message counter
    metrics.heartbeats_sent++;
}

void HeartbeatNode::process_message(const Message& msg) {
    metrics.heartbeats_received++;
    
    if (is_master) {
        // Master node receives heartbeats from workers
        update_node_state(msg.from_id, true);
    } else {
        // Worker nodes receive heartbeat responses from master
        // In this simple implementation, we don't need to do anything with the response
    }
}

void HeartbeatNode::periodic_task() {
    static auto last_heartbeat = get_current_time();
    auto now = get_current_time();
    
    if (!is_master) {
        // Worker nodes send heartbeats to master
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat).count() >= heartbeat_interval_ms) {
            send_heartbeat();
            last_heartbeat = now;
        }
    } else {
        // Master node checks worker health
        check_node_health();
    }
}

void HeartbeatNode::send_heartbeat() {
    // Worker nodes send heartbeats to master
    if (!is_master) {
        std::string heartbeat_msg = "HEARTBEAT";
        send_message("master", heartbeat_msg);
    }
}

void HeartbeatNode::check_node_health() {
    auto now = get_current_time();
    std::lock_guard<std::mutex> lock(states_mutex);
    
    for (auto& [id, state] : node_states) {
        if (id != this->id) {  // Don't check self
            auto time_since_last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - state.last_heartbeat).count();
            
            if (time_since_last_heartbeat > failure_threshold_ms) {
                if (state.is_alive) {
                    state.is_alive = false;
                    metrics.false_positives++;  // This might be a false positive
                }
            }
        }
    }
}

void HeartbeatNode::update_node_state(const std::string& node_id, bool is_alive) {
    std::lock_guard<std::mutex> lock(states_mutex);
    auto it = node_states.find(node_id);
    if (it != node_states.end()) {
        it->second.is_alive = is_alive;
        it->second.last_heartbeat = get_current_time();
    }
}

std::vector<std::string> HeartbeatNode::get_failed_nodes() const {
    std::vector<std::string> failed;
    std::lock_guard<std::mutex> lock(states_mutex);
    
    for (const auto& [id, state] : node_states) {
        if (!state.is_alive) {
            failed.push_back(id);
        }
    }
    
    return failed;
}

void HeartbeatNode::add_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(states_mutex);
    node_states[node_id] = {true, get_current_time()};
}

void HeartbeatNode::remove_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(states_mutex);
    node_states.erase(node_id);
}

HeartbeatNode::Metrics HeartbeatNode::get_metrics() const {
    return metrics;
}

void HeartbeatNode::reset_metrics() {
    metrics = {0, 0, 0, 0, get_current_time()};
} 