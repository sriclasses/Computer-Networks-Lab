# Router Switch Fabric Simulation

## Overview

This project simulates a network router switch fabric designed to handle high-throughput traffic. It explores and compares the performance of four scheduling algorithms: Priority Scheduling, Weighted Fair Queuing (WFQ), Round Robin (RR), and iSLIP. The simulation environment is configured with multiple input and output ports, and it assesses each algorithm's performance under different traffic patterns.

## Objectives

- Implement and analyze four scheduling algorithms for output queuing in router switch fabrics.
- Evaluate each algorithm based on key performance metrics.
- Provide insights into the handling of high-priority and low-priority traffic.
- Recommend the best scheduling algorithm for high-throughput environments.

## Simulation Environment

- **Number of Input Ports:** 8
- **Number of Output Ports:** 8
- **Packet Arrival Rate:** Variable, with high-priority and low-priority traffic.
- **Traffic Patterns:**
  - Uniform Traffic: Equal traffic across all input ports.
  - Non-Uniform Traffic: Higher priority for certain input ports.
  - Bursty Traffic: Random bursts of traffic for specific ports.
- **Buffer Size:** 64 packets for each input/output port.

## Performance Metrics

The following metrics are used to compare the performance of each scheduling algorithm:

1. **Queue Throughput:** Number of packets processed per unit time.
2. **Turnaround Time:** Total time taken from packet arrival to exit.
3. **Waiting Time:** Total time spent waiting in the queue.
4. **Buffer Occupancy:** Track occupancy levels in input/output queues.
5. **Packet Drop Rate:** Percentage of packets dropped due to overflow.

## Algorithms Implemented

- **Priority Scheduling:** Prioritizes packets based on defined criteria, ensuring high-priority traffic is handled first.
- **Weighted Fair Queuing (WFQ):** Allocates bandwidth based on weights assigned to different flows.
- **Round Robin (RR):** Assigns equal time slices to each input port in a cyclic order.
- **iSLIP:** An improvement over traditional round-robin, providing fairness while reducing delays for high-priority traffic.

## Results and Analysis

### Comparison of Algorithms

- **Packet Delay:** Discuss which algorithm achieves the lowest packet delay and the underlying reasons.
- **Traffic Handling:** Analyze how each algorithm manages high-priority versus low-priority traffic.
- **Fairness:** Identify which algorithm provides the highest fairness and how the iSLIP algorithm improves performance over RR.

## Compilation and Execution

g++ pq_final.cpp
g++ rr_final.cpp
g++ wfq_final.cpp
g++ islip_final.cpp

./a.out pq_final.cpp
./a.out rr_final.cpp
./a.out wfq_final.cpp
./a.out islip_final.cpp

For logging data into the output.txt for the islip simulation
./a.out islip_final.cpp > output.log 2>&1

In case the program does not terminate use ctrl+c to terminate the code

## Conclusion

This project highlights the importance of selecting an appropriate scheduling algorithm in network router switch fabrics. The simulations provide valuable insights into the performance characteristics of each algorithm, facilitating better decision-making for network design.
