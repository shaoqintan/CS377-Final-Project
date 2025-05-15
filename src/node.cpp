#include "node.hpp"

using namespace std;

Node::Node(const string& node_id) 
    : id(node_id), is_alive(true), is_running(false) {}

Node::~Node() {
    stop();
}

void Node::stop() {
    is_running = false;
    if (node_thread.joinable()) {
        node_thread.join();
    }
}

void Node::receive_message(const string& from_id, const string& content) {
    Message msg{from_id, content, get_current_time()};
    lock_guard<mutex> lock(queue_mutex);
    message_queue.push(msg);
}

void Node::process_message_queue() {
    lock_guard<mutex> lock(queue_mutex);
    while (!message_queue.empty()) {
        Message msg = message_queue.front();
        message_queue.pop();
        process_message(msg);
    }
}

void Node::run() {
    while (is_running) {
        process_message_queue();
        periodic_task();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

chrono::system_clock::time_point Node::get_current_time() const {
    return chrono::system_clock::now();
} 