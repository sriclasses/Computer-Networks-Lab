# Communication Systems Project

This project includes two distinct communication systems designed for different monitoring and control applications.

## Table of Contents
- [Task 1: Drone Communication System](#task-1-drone-communication-system)
- [Task 2: Weather Monitoring System](#task-2-weather-monitoring-system)

---

## Task 1: Drone Communication System

A monitoring and control system for a fleet of autonomous drones, handling control commands, telemetry data, and file transfers.

### Features
- Handles control commands, telemetry data, and file transfers between the central server and drones
- Ensures reliable communication with error handling and reconnection mechanisms
- Multi-threaded architecture for concurrent drone management

### How to Build

1. **Compile the server and client programs:**
   ```bash
   g++ -o server1 server1.cpp -lpthread
   g++ -o client1 client1.cpp -lpthread
   ```

2. **Run the server:**
   ```bash
   ./server1
   ```

3. **Run the client:**
   For each drone client, execute:
   ```bash
   ./client1 droneId
   ```

---

## Task 2: Weather Monitoring System

A real-time weather monitoring system for a large city, with a central server and multiple weather stations transmitting data such as temperature, humidity, and air pressure.

### Features
- Implements TCP Reno for congestion control to adjust the data transmission rate
- Simulates a limited network environment with constrained bandwidth and server capacity
- Supports dynamic data compression to optimize data transmission without overloading the server
- Real-time weather data collection and processing

### How to Build

1. **Compile the server and client programs:**
   ```bash
   g++ -o server server.cpp -pthread -lz
   g++ -o client client.cpp -lz
   ```

2. **Run the server in the background:**
   ```bash
   ./server > server_output.log 2>&1 &
   ```

3. **Run the client:**
   For each weather station, execute:
   ```bash
   ./run_clients.sh
   ```

---

## Prerequisites

- **Compiler:** GCC with C++11 support or later
- **Libraries:** 
  - pthread (POSIX threads)
  - zlib (for compression in Task 2)
- **Operating System:** Linux/Unix-based systems

## Project Structure

```
project/
├── server1.cpp          # Drone communication server
├── client1.cpp          # Drone communication client
├── server.cpp           # Weather monitoring server
├── client.cpp           # Weather monitoring client
├── run_clients.sh       # Script to run weather station clients
└── README.md           # This file
```

## Usage Notes

- Ensure all required libraries are installed before compilation
- Run the server before starting any clients
- For Task 1, replace `droneId` with the actual identifier for each drone
- Server logs for Task 2 are saved to `server_output.log`
- Both systems support multiple concurrent clients
