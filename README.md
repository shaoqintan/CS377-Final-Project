# Distributed System Failure Detection Simulation

This project implements and compares two failure detection algorithms in a simulated distributed system:
1. Gossip-style failure detection (epidemic protocol)
2. Heartbeat failure detection (centralized approach)

## Project Structure
- `node.py`: Base Node class implementation
- `gossip_node.py`: Gossip-style failure detection implementation
- `heartbeat_node.py`: Heartbeat failure detection implementation
- `network.py`: Network simulation and management
- `simulator.py`: Main simulation runner
- `tests/`: Test cases and scenarios
- `utils.py`: Utility functions and metrics collection

## Setup
1. Create a virtual environment:
```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
```

2. Install dependencies:
```bash
pip install -r requirements.txt
```

## Running the Simulation
```bash
python simulator.py
```

## Running Tests
```bash
pytest tests/
```

## Metrics Collected
1. Latency/Response time
2. Accuracy (true positives, false positives, false negatives)
3. Scalability measurements
4. Network traffic analysis

## Test Scenarios
1. Single Node Failure
2. Multiple Concurrent Failures
3. Temporary Network Partition
4. High Load Scenario
5. Recovery After Failure 