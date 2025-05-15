#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <functional>

using namespace std;

class Node {
protected:
    string id;
    atomic<bool> is_alive;
    atomic<bool> is_running;
    mutex state_mutex;
    thread node_thread;
    
    // Message queue for thread-safe communication
    struct Message {
        string from_id;
        string content;
        chrono::system_clock::time_point timestamp;
    };
    queue<Message> message_queue;
    mutex queue_mutex;

public:
    Node(const string& node_id);
    virtual ~Node();

    // Core functionality
    virtual void start() = 0;
    virtual void stop();
    virtual void send_message(const string& to_id, const string& content) = 0;
    virtual void receive_message(const string& from_id, const string& content);
    
    // State management
    bool is_node_alive() const { return is_alive; }
    void set_alive(bool status) { is_alive = status; }
    string get_id() const { return id; }

    // Message processing
    virtual void process_message(const Message& msg) = 0;
    void process_message_queue();

    virtual vector<string> get_failed_nodes() const = 0;  // Pure virtual method

protected:
    // Helper functions
    void run();
    virtual void periodic_task() = 0;
    chrono::system_clock::time_point get_current_time() const;
}; 