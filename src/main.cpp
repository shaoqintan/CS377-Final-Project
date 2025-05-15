#include "simulator.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;

void run_network_tests(Simulator& simulator, int size, const string& network_type) {
    cout << "\n=== " << network_type << " Network Tests with " << size << " nodes ===\n";
    cout << "------------------------------------------\n";
    
    auto single_failure = simulator.run_single_node_failure_test(size);
    auto multiple_failures = simulator.run_multiple_failures_test(size, size / 2);
    auto network_partition = simulator.run_network_partition_test(size);
    auto high_load = simulator.run_high_load_test(size);
    auto recovery = simulator.run_recovery_test(size);
    
    cout << fixed << setprecision(2);
    
    cout << "\nSingle Node Failure Test:\n"
              << "  Detection Time: " << setw(8) << single_failure.detection_time_ms << " ms\n"
              << "  Accuracy:      " << setw(8) << (single_failure.accuracy * 100) << " %\n"
              << "  Messages Sent: " << setw(8) << single_failure.messages_sent << "\n"
              << "  False Positives: " << single_failure.false_positives << "\n"
              << "  False Negatives: " << single_failure.false_negatives << "\n\n";
    
    cout << "Multiple Failures Test:\n"
              << "  Detection Time: " << setw(8) << multiple_failures.detection_time_ms << " ms\n"
              << "  Accuracy:      " << setw(8) << (multiple_failures.accuracy * 100) << " %\n"
              << "  Messages Sent: " << setw(8) << multiple_failures.messages_sent << "\n"
              << "  False Positives: " << multiple_failures.false_positives << "\n"
              << "  False Negatives: " << multiple_failures.false_negatives << "\n\n";
    
    cout << "Network Partition Test:\n"
              << "  Detection Time: " << setw(8) << network_partition.detection_time_ms << " ms\n"
              << "  Accuracy:      " << setw(8) << (network_partition.accuracy * 100) << " %\n"
              << "  Messages Sent: " << setw(8) << network_partition.messages_sent << "\n"
              << "  False Positives: " << network_partition.false_positives << "\n"
              << "  False Negatives: " << network_partition.false_negatives << "\n\n";
    
    cout << "High Load Test:\n"
              << "  Detection Time: " << setw(8) << high_load.detection_time_ms << " ms\n"
              << "  Accuracy:      " << setw(8) << (high_load.accuracy * 100) << " %\n"
              << "  Messages Sent: " << setw(8) << high_load.messages_sent << "\n"
              << "  False Positives: " << high_load.false_positives << "\n"
              << "  False Negatives: " << high_load.false_negatives << "\n\n";
    
    cout << "Recovery Test:\n"
              << "  Detection Time: " << setw(8) << recovery.detection_time_ms << " ms\n"
              << "  Accuracy:      " << setw(8) << (recovery.accuracy * 100) << " %\n"
              << "  Messages Sent: " << setw(8) << recovery.messages_sent << "\n"
              << "  False Positives: " << recovery.false_positives << "\n"
              << "  False Negatives: " << recovery.false_negatives << "\n";
    
    cout << "------------------------------------------\n";
}

int main() {
    srand(time(nullptr));
    
    Simulator simulator;
    
    vector<int> network_sizes = {5, 10, 20, 50};
    
    for (int size : network_sizes) {
        cout << "\n==========================================\n";
        cout << "TESTING WITH " << size << " NODES\n";
        cout << "==========================================\n";
        
        simulator.setup_gossip_network(size);
        run_network_tests(simulator, size, "Gossip");
        simulator.cleanup_network();
        
        simulator.setup_heartbeat_network(size);
        run_network_tests(simulator, size, "Heartbeat");
        simulator.cleanup_network();
        
        cout << "\nAlgorithm Comparison Results:\n";
        cout << "------------------------------------------\n";
        simulator.compare_algorithms(size);
        cout << "------------------------------------------\n";
    }
    
    return 0;
} 