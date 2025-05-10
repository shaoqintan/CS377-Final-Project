#include "simulator.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <iomanip>

void run_network_tests(Simulator& simulator, int size, const std::string& network_type) {
    std::cout << "\n=== " << network_type << " Network Tests with " << size << " nodes ===\n";
    std::cout << "------------------------------------------\n";
    
    // Run all test scenarios
    auto single_failure = simulator.run_single_node_failure_test(size);
    auto multiple_failures = simulator.run_multiple_failures_test(size, size / 2);
    auto network_partition = simulator.run_network_partition_test(size);
    auto high_load = simulator.run_high_load_test(size);
    auto recovery = simulator.run_recovery_test(size);
    
    // Print results with better formatting
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\nSingle Node Failure Test:\n"
              << "  Detection Time: " << std::setw(8) << single_failure.detection_time_ms << " ms\n"
              << "  Accuracy:      " << std::setw(8) << (single_failure.accuracy * 100) << " %\n"
              << "  Messages Sent: " << std::setw(8) << single_failure.messages_sent << "\n"
              << "  False Positives: " << single_failure.false_positives << "\n"
              << "  False Negatives: " << single_failure.false_negatives << "\n\n";
    
    std::cout << "Multiple Failures Test:\n"
              << "  Detection Time: " << std::setw(8) << multiple_failures.detection_time_ms << " ms\n"
              << "  Accuracy:      " << std::setw(8) << (multiple_failures.accuracy * 100) << " %\n"
              << "  Messages Sent: " << std::setw(8) << multiple_failures.messages_sent << "\n"
              << "  False Positives: " << multiple_failures.false_positives << "\n"
              << "  False Negatives: " << multiple_failures.false_negatives << "\n\n";
    
    std::cout << "Network Partition Test:\n"
              << "  Detection Time: " << std::setw(8) << network_partition.detection_time_ms << " ms\n"
              << "  Accuracy:      " << std::setw(8) << (network_partition.accuracy * 100) << " %\n"
              << "  Messages Sent: " << std::setw(8) << network_partition.messages_sent << "\n"
              << "  False Positives: " << network_partition.false_positives << "\n"
              << "  False Negatives: " << network_partition.false_negatives << "\n\n";
    
    std::cout << "High Load Test:\n"
              << "  Detection Time: " << std::setw(8) << high_load.detection_time_ms << " ms\n"
              << "  Accuracy:      " << std::setw(8) << (high_load.accuracy * 100) << " %\n"
              << "  Messages Sent: " << std::setw(8) << high_load.messages_sent << "\n"
              << "  False Positives: " << high_load.false_positives << "\n"
              << "  False Negatives: " << high_load.false_negatives << "\n\n";
    
    std::cout << "Recovery Test:\n"
              << "  Detection Time: " << std::setw(8) << recovery.detection_time_ms << " ms\n"
              << "  Accuracy:      " << std::setw(8) << (recovery.accuracy * 100) << " %\n"
              << "  Messages Sent: " << std::setw(8) << recovery.messages_sent << "\n"
              << "  False Positives: " << recovery.false_positives << "\n"
              << "  False Negatives: " << recovery.false_negatives << "\n";
    
    std::cout << "------------------------------------------\n";
}

int main() {
    // Initialize random seed
    std::srand(std::time(nullptr));
    
    // Create simulator
    Simulator simulator;
    
    // Run tests with different network sizes
    std::vector<int> network_sizes = {5, 10, 20, 50};
    
    for (int size : network_sizes) {
        std::cout << "\n==========================================\n";
        std::cout << "TESTING WITH " << size << " NODES\n";
        std::cout << "==========================================\n";
        
        // Run tests on Gossip Network
        simulator.setup_gossip_network(size);
        run_network_tests(simulator, size, "Gossip");
        simulator.cleanup_network();
        
        // Run tests on Heartbeat Network
        simulator.setup_heartbeat_network(size);
        run_network_tests(simulator, size, "Heartbeat");
        simulator.cleanup_network();
        
        // Run algorithm comparison
        std::cout << "\nAlgorithm Comparison Results:\n";
        std::cout << "------------------------------------------\n";
        simulator.compare_algorithms(size);
        std::cout << "------------------------------------------\n";
    }
    
    return 0;
} 