# Distance Vector Routing (DVR) Algorithm Simulation

## Overview

This project implements a simulation of the Distance Vector Routing (DVR) algorithm to calculate shortest paths in a network of routers. It also includes fault tolerance and loop prevention techniques, such as Poisoned Reverse and Split Horizon, to handle link failures and avoid routing loops.

## Objectives

- Implement the DVR algorithm for calculating shortest paths across a network.
- Simulate the effects of a network link failure and observe the impact on routing tables.
- Apply Poisoned Reverse and Split Horizon techniques to prevent routing loops and the count-to-infinity problem.
- Analyze and compare routing tables before and after implementing these techniques.

## Simulation Environment

- Graph Setup: Configured with nodes (routers) and edges (network links).
- Link Failure Simulation: An edge (e.g., between nodes 5 and 4) is removed to test the count-to-infinity issue.
- Count-to-Infinity Threshold: The simulation halts if the distance exceeds 100 to prevent endless loops.
- Routing Table Outputs:
   Initial DVR routing tables for each node.
   Updated routing tables after link failure.
   Routing tables after applying Poisoned Reverse.
   Routing tables after applying Split Horizon.

## Algorithms Implemented

- Distance Vector Routing (DVR): Calculates the shortest path between nodes in a network by sharing routing information between neighboring nodes.
- Poisoned Reverse: Sends "infinity" values to prevent back-propagation of failed routes, reducing the count-to-infinity risk.
- Split Horizon: Prevents routes from being advertised back in the direction they were received, thus avoiding loops.

## Compilation and Execution

Compilation : 
make 
OR 
g++ -o DVR_simulation code.cpp

Execution :

./DVR_simulation

## Outputs

- Initial Routing Tables: DVR-calculated shortest paths from each router to all others.
Routing Tables after Link Failure: Updated paths and any indication of count-to-infinity issues.
- Routing Tables with Poisoned Reverse: Routes with modified distances to prevent back-propagation.
- Routing Tables with Split Horizon: Final tables reflecting shortest paths with loop-prevention.

## Conclusion

This project simulates the DVR algorithm with mechanisms to address routing challenges, providing insights into routing table changes due to link failure and evaluating the effectiveness of Poisoned Reverse and Split Horizon techniques in preventing routing loops.