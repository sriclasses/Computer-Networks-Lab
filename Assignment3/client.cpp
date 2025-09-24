#include <iostream>
#include <zlib.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
#include <sys/select.h>
using namespace std;

// Define constants for server connection
#define SERVER_PORT 8090        // The server port to connect to
#define SERVER_IP "127.0.0.1"   // The server's IP address (localhost)
#define BUFFER_SIZE 1024        // Buffer size for sending/receiving data
#define DATA_COUNT 10           // Number of data packets to send
#define TIMEOUT_SECONDS 2       // Timeout (in seconds) for waiting for ACK from server
#define MAX_RETRIES 5           // Maximum number of retries for each packet

// Global variables related to TCP Reno congestion control
int tcp_reno_cwnd = 1;          // Congestion window starts with 1 (slow start phase)
int ssthresh = 16;              // Slow start threshold for transitioning to congestion avoidance
int duplicate_ack_count = 0;    // To track duplicate ACKs (for fast retransmit)

// Function to get the current timestamp (with milliseconds) for logging purposes
string currentTimestamp() {
    auto now = chrono::system_clock::now();  // Get the current time
    auto now_time_t = chrono::system_clock::to_time_t(now);  // Convert to time_t format
    auto now_ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;  // Extract milliseconds

    tm *now_tm = localtime(&now_time_t);  // Convert time to a human-readable format
    stringstream timestamp;
    timestamp << put_time(now_tm, "%Y-%m-%d %H:%M:%S");  // Format date and time
    timestamp << "." << setfill('0') << setw(3) << now_ms.count();  // Append milliseconds to the timestamp
    return timestamp.str();  // Return the formatted timestamp
}

// Simulate generation of random weather data (temperature, humidity, pressure)
string generateWeatherData() {
    // Generate random temperature, humidity, and pressure values
    double temperature = (rand() % 2000) / 100.0 + 20.0;
    double humidity = (rand() % 4000) / 100.0 + 30.0;
    double pressure = (rand() % 20000) / 100.0 + 900.0;
    // Return the generated data as a formatted string
    return "Temperature: " + to_string(temperature) + "C, Humidity: " + to_string(humidity) + "%, Pressure: " + to_string(pressure) + "hPa";
}

// Function to choose the zlib compression level dynamically based on data variability
int chooseCompressionLevel(const string& data) {
    size_t repeated_pattern_count = 0;  // Counter for repeated patterns in data
    // Loop through the data to count how many consecutive characters are the same
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] == data[i - 1]) {
            repeated_pattern_count++;
        }
    }
    // If more than 1/3 of the data is repetitive, choose a high compression level (9), otherwise low compression (1)
    return (repeated_pattern_count > data.size() / 3) ? 9 : 1;
}

// Function to compress data using zlib with a specified compression level
string compressData(const string &data, int compression_level, size_t &compressed_size) {
    uLongf compressed_length = compressBound(data.size());  // Get the upper bound for compressed size
    vector<char> compressed_data(compressed_length);   // Create a buffer to hold the compressed data
    // Compress the data using the zlib library
    compress2((Bytef *)compressed_data.data(), &compressed_length, (Bytef *)data.c_str(), data.size(), compression_level);
    compressed_size = compressed_length;  // Update the actual compressed size
    return string(compressed_data.data(), compressed_length);  // Return the compressed data as a string
}

// Function to simulate TCP Reno congestion control algorithm behavior based on packet success/failure
void tcpRenoCongestionControl(bool success) {
    if (success) {
        if (tcp_reno_cwnd < ssthresh) {
            tcp_reno_cwnd *= 2;  // In slow start phase, congestion window doubles
            cout << "[" << currentTimestamp() << "] Slow start phase: cwnd doubled to " << tcp_reno_cwnd << endl;
        } else {
            tcp_reno_cwnd += 1;  // In congestion avoidance phase, window grows linearly
            cout << "[" << currentTimestamp() << "] Congestion avoidance phase: cwnd increased to " << tcp_reno_cwnd << endl;
        }
    } else {
        ssthresh = tcp_reno_cwnd / 2;  // On packet loss, halve the slow start threshold (ssthresh)
        tcp_reno_cwnd = 1;  // Reset congestion window to 1 (go back to slow start)
        cout << "[" << currentTimestamp() << "] Packet loss detected. Resetting cwnd to 1 and ssthresh to " << ssthresh << endl;
        duplicate_ack_count = 0;  // Reset the duplicate ACK counter
    }
}

// Function to wait for a specific ACK packet from the server, with timeout handling
bool waitForAck(int client_socket, int packet_id) {
    fd_set read_fds;  // File descriptor set to track readability of client_socket
    FD_ZERO(&read_fds);  // Clear the set
    FD_SET(client_socket, &read_fds);  // Add client_socket to the set

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;  // Set the timeout value (in seconds)
    timeout.tv_usec = 0;  // Microseconds set to 0

    int select_result = select(client_socket + 1, &read_fds, nullptr, nullptr, &timeout);  // Use select() to wait for data
    if (select_result == 0) {
        // If select() times out (no data received)
        cout << "[" << currentTimestamp() << "] Timeout occurred waiting for ACK." << endl;
        return false;
    } else if (select_result < 0) {
        // If an error occurred with select()
        cerr << "[" << currentTimestamp() << "] Error in select() during ACK wait." << endl;
        return false;
    }

    // If data is available to read, check if it is the expected ACK
    char ack_buffer[BUFFER_SIZE] = {0};  // Buffer to store the received ACK message
    ssize_t ack_received = recv(client_socket, ack_buffer, sizeof(ack_buffer), 0);  // Receive the ACK from server
    
    string expected_ack = "ACK " + to_string(packet_id);  // Form the expected ACK message
    if (ack_received > 0 && string(ack_buffer, ack_received) == expected_ack) {
        // If the received ACK matches the expected ACK
        cout << "[" << currentTimestamp() << "] Correct ACK (" << expected_ack << ") received." << endl;
        return true;  // Return true if valid ACK is received
    } else {
        // If the received ACK is incorrect or not received
        cout << "[" << currentTimestamp() << "] Incorrect or no ACK received. Expected: " << expected_ack << ", Received: " << string(ack_buffer, ack_received) << endl;
        return false;
    }
}

// Function to send a packet and handle retransmissions in case of timeout or incorrect ACK
bool sendWithTimeout(int client_socket, const string &data, int packet_id) {
    int retries = 0;  // Initialize retry counter
    while (retries < MAX_RETRIES) {
        // Attach packet ID to data before sending it
        string packet_with_id = to_string(packet_id) + ":" + data;

        // Send the packet to the server
        ssize_t sent_bytes = send(client_socket, packet_with_id.c_str(), packet_with_id.size(), 0);
        if (sent_bytes < 0) {
            // If packet sending fails
            cerr << "[" << currentTimestamp() << "] Error sending packet " << packet_id << endl;
            return false;
        }

        // Wait for ACK or timeout
        if (!waitForAck(client_socket, packet_id)) {
            retries++;  // Increment retry count if no ACK or incorrect ACK received
            cout << "[" << currentTimestamp() << "] Retransmitting packet " << packet_id << " (Retry #" << retries << ")" << endl;
            tcpRenoCongestionControl(false);  // Simulate packet loss by invoking congestion control
        } else {
            // If ACK is received successfully
            cout << "[" << currentTimestamp() << "] ACK received for packet " << packet_id << endl;
            tcpRenoCongestionControl(true);  // Update congestion control for successful transmission
            return true;
        }
    }

    // If the packet fails to send even after retries
    cerr << "[" << currentTimestamp() << "] Packet " << packet_id << " failed to send after " << retries << " retries." << endl;
    return false;
}

// Main function to simulate data transmission over TCP using congestion control and compression
int main() {
    srand(time(0));  // Seed random number generator

    // Create a TCP socket for connecting to the server
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "[" << currentTimestamp() << "] Failed to create socket." << endl;
        return 1;
    }

    // Set up server address structure for connecting to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);  // Convert port number to network byte order
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);  // Convert IP address to binary form

    // Attempt to connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "[" << currentTimestamp() << "] Connection to server failed." << endl;
        close(client_socket);  // Close the socket if connection fails
        return 1;
    }

    cout << "[" << currentTimestamp() << "] Connected to server." << endl;

    // Loop through data packets, generate weather data, compress, and send to server
    for (int packet_id = 1; packet_id <= DATA_COUNT; ++packet_id) {
        string data = generateWeatherData();  // Generate random weather data

        // Determine compression level based on data variability
        int compression_level = chooseCompressionLevel(data);

        // Compress the data
        size_t compressed_size = 0;
        string compressed_data = compressData(data, compression_level, compressed_size);

        // Send compressed data with timeout and retries
        if (!sendWithTimeout(client_socket, compressed_data, packet_id)) {
            cerr << "[" << currentTimestamp() << "] Failed to send packet " << packet_id << " after retries." << endl;
        } else {
            cout << "[" << currentTimestamp() << "] Successfully sent packet " << packet_id << " with compressed size " << compressed_size << " bytes." << endl;
        }

        // Wait before sending the next packet to simulate real-time transmission
        this_thread::sleep_for(chrono::seconds(1));
    }

    cout << "[" << currentTimestamp() << "] All data packets sent. Closing connection." << endl;
    close(client_socket);  // Close the socket after transmission
    return 0;
}
