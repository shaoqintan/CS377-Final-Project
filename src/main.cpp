#include "simulator.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    // Initialize random seed
    std::srand(std::time(nullptr));
    
    // Create simulator
    Simulator simulator;
    
    // Run tests with different network sizes
    std::vector<int> network_sizes = {5, 10, 20, 50};
    
    for (int size : network_sizes) {
        std::cout << "\n=== Testing with " << size << " nodes ===\n";
        
        // Run all test scenarios
        auto single_failure = simulator.run_single_node_failure_test(size);
        auto multiple_failures = simulator.run_multiple_failures_test(size, size / 2);
        auto network_partition = simulator.run_network_partition_test(size);
        auto high_load = simulator.run_high_load_test(size);
        auto recovery = simulator.run_recovery_test(size);
        
        // Print results
        std::cout << "\nSingle Node Failure Test:\n"
                  << "Detection Time: " << single_failure.detection_time_ms << "ms\n"
                  << "Accuracy: " << (single_failure.accuracy * 100) << "%\n"
                  << "Messages Sent: " << single_failure.messages_sent << "\n\n";
        
        std::cout << "Multiple Failures Test:\n"
                  << "Detection Time: " << multiple_failures.detection_time_ms << "ms\n"
                  << "Accuracy: " << (multiple_failures.accuracy * 100) << "%\n"
                  << "Messages Sent: " << multiple_failures.messages_sent << "\n\n";
        
        std::cout << "Network Partition Test:\n"
                  << "Detection Time: " << network_partition.detection_time_ms << "ms\n"
                  << "Accuracy: " << (network_partition.accuracy * 100) << "%\n"
                  << "Messages Sent: " << network_partition.messages_sent << "\n\n";
        
        std::cout << "High Load Test:\n"
                  << "Detection Time: " << high_load.detection_time_ms << "ms\n"
                  << "Accuracy: " << (high_load.accuracy * 100) << "%\n"
                  << "Messages Sent: " << high_load.messages_sent << "\n\n";
        
        std::cout << "Recovery Test:\n"
                  << "Detection Time: " << recovery.detection_time_ms << "ms\n"
                  << "Accuracy: " << (recovery.accuracy * 100) << "%\n"
                  << "Messages Sent: " << recovery.messages_sent << "\n";
    }
    
    return 0;
} 