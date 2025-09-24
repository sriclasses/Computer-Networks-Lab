#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <zlib.h>                 // For data compression and decompression
#include <cstring>
#include <netinet/in.h>           // For socket programming (in network communication)
#include <unistd.h>               // For POSIX system calls (close, etc.)
#include <mutex>                  // For thread synchronization
#include <condition_variable>     // For managing task queue notifications
#include <chrono>                 // For handling time-related operations
#include <iomanip>                // For formatted output (e.g., timestamps)
#include <sstream>                // For converting timestamp data to strings
using namespace std;
#define PORT 8090                 // The port number on which the server listens for connections
#define BUFFER_SIZE 1024          // The size of the buffer to hold incoming data
#define PACKET_LOSS_PROBABILITY 0.2  // Simulated 20% packet loss probability
#define MAX_QUEUE_SIZE 10         // Maximum size of the task queue (for data processing)
#define BANDWIDTH_LOW 100         // Simulated low bandwidth (100 bytes/second)
#define BANDWIDTH_HIGH 1000       // Simulated high bandwidth (1000 bytes/second)

mutex queue_mutex;           // Mutex to protect access to the task queue
condition_variable queue_cond_var;  // Condition variable for task queue management
queue<pair<int, string>> task_queue;  // Queue holding client tasks (client ID and data)

mutex log_mutex;             // Mutex to ensure thread-safe logging
int client_count = 0;             // Counter for connected clients
bool stop_server = false;         // Flag to signal the server to stop
int bandwidth = BANDWIDTH_HIGH;   // Initial bandwidth, starts with high bandwidth

// Function to get the current timestamp (including milliseconds)
string currentTimestamp() {
    auto now = chrono::system_clock::now();
    auto now_time_t = chrono::system_clock::to_time_t(now);
    auto now_ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

    tm *now_tm = localtime(&now_time_t);
    stringstream timestamp;
    timestamp << put_time(now_tm, "%Y-%m-%d %H:%M:%S");
    timestamp << "." << setfill('0') << setw(3) << now_ms.count();
    return timestamp.str();
}

// Thread-safe logging function to ensure multiple threads can log without conflicts
void logMessage(const string& message) {
    lock_guard<mutex> lock(log_mutex);
    cout << "[" << currentTimestamp() << "] " << message << endl;
}

// Simulate bandwidth throttling by adding delays based on the amount of data processed
void simulateBandwidthThrottling(size_t bytesProcessed) {
    int delay = (bytesProcessed * 1000) / bandwidth; // Delay in milliseconds based on current bandwidth
    if (delay >= 2000) { // If delay is large enough, log that it could cause a timeout
        logMessage("Simulating bandwidth delay that could cause timeout: " + to_string(delay) + "ms");
    }
    this_thread::sleep_for(chrono::milliseconds(delay)); // Sleep for the calculated delay time
}

// Simulate packet loss based on a defined probability (using random numbers)
bool simulatePacketLoss() {
    double rand_val = static_cast<double>(rand()) / RAND_MAX; // Generate random number between 0 and 1
    return rand_val < PACKET_LOSS_PROBABILITY; // If random number is less than the loss probability, simulate loss
}

// Decompress the data received from the client using zlib library
string decompressData(const string &compressed_data, size_t &decompressed_size) {
    uLongf decompressed_length = BUFFER_SIZE;
    vector<char> decompressed_data(BUFFER_SIZE); // Buffer to hold decompressed data
    int result = uncompress((Bytef *)decompressed_data.data(), &decompressed_length, (const Bytef *)compressed_data.c_str(), compressed_data.size());
    decompressed_size = decompressed_length; // Update the decompressed size
    return (result == Z_OK) ? string(decompressed_data.data(), decompressed_length) : ""; // Return decompressed data if successful
}

// Function that runs in worker threads to process tasks from the task queue
void processTask() {
    while (!stop_server) {
        unique_lock<mutex> lock(queue_mutex); // Lock task queue
        queue_cond_var.wait(lock, [] { return !task_queue.empty() || stop_server; }); // Wait until tasks are available or server is stopping

        if (!task_queue.empty()) {
            auto task = task_queue.front();  // Get task from the front of the queue
            task_queue.pop();  // Remove task from the queue
            lock.unlock();     // Unlock the queue after processing the task

            int client_id = task.first;      // Get client ID
            string compressed_data = task.second;  // Get compressed data

            simulateBandwidthThrottling(compressed_data.size());  // Simulate throttling based on data size

            size_t decompressed_size;
            string decompressed_data = decompressData(compressed_data, decompressed_size);  // Decompress the data

            if (!decompressed_data.empty()) {  // If decompression was successful
                logMessage("Client " + to_string(client_id) + " sent weather data: " + decompressed_data);
                logMessage("Compressed size: " + to_string(compressed_data.size()) + " bytes, Decompressed size: " + to_string(decompressed_size) + " bytes.");
            } else {
                logMessage("Decompression error from Client " + to_string(client_id));
            }

            // Dynamically switch between high and low bandwidth to simulate bandwidth changes
            bandwidth = (bandwidth == BANDWIDTH_HIGH) ? BANDWIDTH_LOW : BANDWIDTH_HIGH;
        }
    }
}

// Function to handle communication with each client
void handleClient(int client_socket, int client_id) {
    char buffer[BUFFER_SIZE];
    int packet_id = 1;  // Track packet IDs for this client

    while (true) {
        int bytesReceived = recv(client_socket, buffer, BUFFER_SIZE, 0);  // Receive data from the client
        if (bytesReceived <= 0) {
            break;  // If no data is received, exit the loop
        }

        // Extract the packet ID from the received data
        string received_data(buffer, bytesReceived);
        size_t colon_pos = received_data.find(':');  // Find the separator between packet ID and data
        if (colon_pos == string::npos) {
            logMessage("Malformed packet from Client " + to_string(client_id));  // Log malformed packet
            continue;
        }

        int received_packet_id = stoi(received_data.substr(0, colon_pos));  // Extract packet ID
        string compressed_data = received_data.substr(colon_pos + 1);  // Extract compressed data

        // Simulate packet loss
        if (simulatePacketLoss()) {
            logMessage("Simulating packet loss for Packet " + to_string(received_packet_id) + " from Client " + to_string(client_id));
            continue;  // Don't acknowledge lost packets
        }

        unique_lock<mutex> lock(queue_mutex);
        if (task_queue.size() < MAX_QUEUE_SIZE) {
            // Add task to the queue if there's space
            task_queue.push({client_id, compressed_data});
            queue_cond_var.notify_one();  // Notify a waiting worker thread

            // Log that the packet was successfully received and added to the queue
            logMessage("Received Packet " + to_string(received_packet_id) + " from Client " + to_string(client_id));

            // Send acknowledgment back to the client
            string ack_message = "ACK " + to_string(received_packet_id);
            ssize_t sent_ack = send(client_socket, ack_message.c_str(), ack_message.size(), 0);
            if (sent_ack < 0) {
                logMessage("Failed to send ACK to Client " + to_string(client_id));
            } else {
                logMessage("Sent ACK for Packet " + to_string(received_packet_id) + " to Client " + to_string(client_id));
            }
        } else {
            // If task queue is full, log the dropped packet and don't send ACK
            logMessage("Task queue full! Dropping Packet " + to_string(received_packet_id) + " from Client " + to_string(client_id));
        }

        packet_id++;
    }

    logMessage("Client " + to_string(client_id) + " disconnected.");
    close(client_socket);  // Close the client's socket
}


int main() {
    srand(time(0));  // Seed the random number generator for packet loss simulation

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        cerr << "Socket creation failed" << endl;
        exit(EXIT_FAILURE);
    }

    // Configure the server's address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address
    address.sin_port = htons(PORT);        // Use the defined port

    // Bind the socket to the server address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections (with a backlog of 3)
    if (listen(server_fd, 3) < 0) {
        cerr << "Listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    logMessage("Server started on port " + to_string(PORT));

    // Launch worker threads to process incoming tasks
    vector<thread> worker_threads;
    for (int i = 0; i < 4; ++i) {  // Create 4 worker threads
        worker_threads.emplace_back(processTask);
    }

    // Main loop to accept and handle incoming client connections
    while (!stop_server) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket >= 0) {
            client_count++;
            int client_id = client_count;
            logMessage("New client connected. Client ID: " + to_string(client_id));
            thread client_thread(handleClient, new_socket, client_id);  // Handle the new client in a separate thread
            client_thread.detach();  // Detach the thread to allow it to run independently
        }
    }

    // Clean up worker threads after server stops
    stop_server = true;
    queue_cond_var.notify_all();  // Wake up all worker threads so they can exit
    for (auto &t : worker_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    close(server_fd);  // Close the server socket
    return 0;
}
