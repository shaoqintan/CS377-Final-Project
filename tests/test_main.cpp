#include <gtest/gtest.h>
#include "../include/node.hpp"
#include "../include/gossip_node.hpp"
#include "../include/heartbeat_node.hpp"
#include "../include/network.hpp"
#include "../include/simulator.hpp"

// Test Node base class
TEST(NodeTest, BasicFunctionality) {
    Node node("test_node");
    EXPECT_EQ(node.get_id(), "test_node");
    EXPECT_TRUE(node.is_node_alive());
    
    node.set_alive(false);
    EXPECT_FALSE(node.is_node_alive());
}

// Test GossipNode
TEST(GossipNodeTest, BasicFunctionality) {
    std::vector<std::string> peers = {"peer1", "peer2", "peer3"};
    GossipNode node("test_node", peers);
    
    EXPECT_EQ(node.get_id(), "test_node");
    EXPECT_TRUE(node.is_node_alive());
    
    // Test peer management
    node.add_peer("peer4");
    node.remove_peer("peer1");
    
    // Test metrics
    auto metrics = node.get_metrics();
    EXPECT_EQ(metrics.messages_sent, 0);
    EXPECT_EQ(metrics.messages_received, 0);
}

// Test HeartbeatNode
TEST(HeartbeatNodeTest, BasicFunctionality) {
    HeartbeatNode master("master", true);
    HeartbeatNode worker("worker", false);
    
    EXPECT_TRUE(master.is_master_node());
    EXPECT_FALSE(worker.is_master_node());
    
    // Test node management
    master.add_node("worker");
    master.remove_node("worker");
}

// Test Network
TEST(NetworkTest, BasicFunctionality) {
    Network network;
    
    // Test node management
    auto node = std::make_shared<Node>("test_node");
    network.add_node("test_node", node);
    EXPECT_EQ(network.get_node("test_node"), node);
    
    network.remove_node("test_node");
    EXPECT_EQ(network.get_node("test_node"), nullptr);
}

// Test Simulator
TEST(SimulatorTest, BasicFunctionality) {
    Simulator simulator;
    
    // Test single node failure
    auto result = simulator.run_single_node_failure_test(5);
    EXPECT_GT(result.detection_time_ms, 0);
    EXPECT_GE(result.accuracy, 0.0);
    EXPECT_LE(result.accuracy, 1.0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 