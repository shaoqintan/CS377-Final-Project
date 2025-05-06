#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <functional>

class Node {
protected:
    std::string id;
    std::atomic<bool> is_alive;
    std::atomic<bool> is_running;
    std::mutex state_mutex;
    std::thread node_thread;
    
    // Message queue for thread-safe communication
    struct Message {
        std::string from_id;
        std::string content;
        std::chrono::system_clock::time_point timestamp;
    };
    std::queue<Message> message_queue;
    std::mutex queue_mutex;

public:
    Node(const std::string& node_id);
    virtual ~Node();

    // Core functionality
    virtual void start() = 0;
    virtual void stop();
    virtual void send_message(const std::string& to_id, const std::string& content) = 0;
    virtual void receive_message(const std::string& from_id, const std::string& content);
    
    // State management
    bool is_node_alive() const { return is_alive; }
    void set_alive(bool status) { is_alive = status; }
    std::string get_id() const { return id; }

    // Message processing
    virtual void process_message(const Message& msg) = 0;
    void process_message_queue();

protected:
    // Helper functions
    void run();
    virtual void periodic_task() = 0;
    std::chrono::system_clock::time_point get_current_time() const;
}; 