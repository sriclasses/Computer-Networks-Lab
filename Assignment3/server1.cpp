#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <mutex>
#include <map>
#include <sstream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <random>

using namespace std;
#define BUFFER_SIZE 4096
mutex data_mutex;  // Renamed mutex
const std::string log_filename = "server_log.txt";  // Changed filename

// int control_port = 8080;
// int telemetry_port = 8081;
// int file_port = 8082;

int control_port = 9000;
int telemetry_port = 9001;
int file_port = 9002;

#define FILE_CHUNK_SIZE 1024
#define ACK_TIMEOUT 10000  // Timeout in milliseconds
#define CONGESTION_WINDOW 8  // Initial congestion window size

mutex file_mutex;  // Mutex to protect file operations
map<string, int> client_files; // Map to track file transfer states
map<string, int> congestion_control; // Map to control sending rates
char encryption_key = 'K';  // Renamed key

std::string xorEncryptDecrypt(const std::string &data, char key) {
    std::string result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= key;
    }
    return result;
}

vector<char> xor_encrypt(const vector<char>& data, char key) {
    vector<char> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ key;
    }
    return result;
}

// Function to receive file data over UDP with acknowledgments and congestion control
void receive_file_data(int udp_sock, const string& client_id) {
    char buffer[FILE_CHUNK_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    ofstream file("received_file_" + client_id+".bin", ios::binary);

    // Congestion control variables
    int cwnd = CONGESTION_WINDOW;
    chrono::milliseconds rtt(ACK_TIMEOUT);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 10);

    while (true) {
        // Set receive timeout for acknowledgments
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = ACK_TIMEOUT * 1000;
        setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int received_len = recvfrom(udp_sock, buffer, FILE_CHUNK_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (received_len < 0) {
            cout << "Error receiving file data: " << strerror(errno) << endl;
            break;
        }

        if (received_len == 0) {
            // End of file data
            cout << "File transfer complete for client " << client_id << endl;
            break;
        }

        // Decrypt and write data to file
        vector<char> decrypted_data(buffer, buffer + received_len);
        auto decrypted = xor_encrypt(decrypted_data, encryption_key);

        lock_guard<mutex> lock(file_mutex);
        file.write(decrypted.data(), decrypted.size());

        // Simulate congestion control adjustment
        this_thread::sleep_for(chrono::milliseconds(dis(gen) * 100));
        cwnd = max(1, cwnd - 1);  // Simple congestion control: reduce window size

        // Send acknowledgment
        string ack = "ACK";
        sendto(udp_sock, ack.c_str(), ack.size(), 0, (struct sockaddr*)&client_addr, client_addr_len);
    }

    file.close();
}


struct SensorData {
    int x_coord;
    int y_coord;
    int velocity;
    bool is_active;
    string movement_direction;
    SensorData() : x_coord(0), y_coord(0), velocity(0), is_active(false), movement_direction("None") {};
};

map<string, struct sockaddr_in> client_map;

vector<char> encrypt_data(const vector<char>& input, char key) {
    vector<char> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ key;
    }
    return output;
}

SensorData parse_sensor_data(const string& data_str) {
    SensorData data;
    stringstream ss(data_str);
    ss >> data.x_coord >> data.y_coord >> data.velocity >> data.is_active >> data.movement_direction;
    return data;
}

void process_tcp_client(int client_sock) {
    char buffer[BUFFER_SIZE] = {0};
    // Receive client ID
    int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        cerr << "Failed to read data: " << strerror(errno) << endl;
        return;
    }
    string client_id(buffer, bytes_received);
    cout << "Client Connected: " << client_id << endl;

    while (true) {
        int bytes_read = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {  // Handle both errors and disconnection
            cout << "Client " << client_id << " has disconnected." << endl;
            break; 
        }

        vector<char> received_data(buffer, buffer + bytes_read);

        // Decrypt the received data
        auto decrypted_data = encrypt_data(received_data, encryption_key);

        // Convert decrypted data to string
        string decrypted_str(decrypted_data.begin(), decrypted_data.end());

        // Deserialize to SensorData object
        SensorData sensor_info = parse_sensor_data(decrypted_str);

        // Log received data to file
        ofstream log_file;
        log_file.open(log_filename, std::ios::app);
        log_file << "Data from " << client_id << ":" << endl;
        log_file << "Position: (" << sensor_info.x_coord << ", " << sensor_info.y_coord << ")" << endl;
        log_file << "Velocity: " << sensor_info.velocity << endl;
        log_file << "Active: " << (sensor_info.is_active ? "Yes" : "No") << endl;
        log_file << "Direction: " << sensor_info.movement_direction << endl;
        log_file.close();
    }
    close(client_sock);
}

void process_quic_client(int client_sock) {
    char buffer[BUFFER_SIZE] = {0};

    // Receive file name
    std::ofstream file("received_file" + to_string(client_sock) + ".dat", std::ios::out | std::ios::binary);

        // Read file in chunks
        int valread;
        while ((valread = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
            std::string encryptedChunk(buffer, valread);
            std::string chunk = xorEncryptDecrypt(encryptedChunk, 'K');
            file.write(chunk.c_str(), chunk.size());
        }

    file.close();
    close(client_sock);
}

void handle_file(int port) {
    try {
        int server_sock;
        struct sockaddr_in server_addr, client_addr;

        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
            cerr << "Error creating TCP socket: " << strerror(errno) << endl;
            return;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Binding error: " << strerror(errno) << endl;
            close(server_sock);
            return;
        }

        if (listen(server_sock, 5) < 0) {
            cerr << "Listening error: " << strerror(errno) << endl;
            close(server_sock);
            return;
        }

        cout << "QUIC Server running on port " << port << endl;
        map<int, thread> client_threads;
        socklen_t addr_len = sizeof(client_addr);

        while (true) {
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock < 0) {
                cerr << "Error accepting connection: " << strerror(errno) << endl;
                continue;
            }

            cout << "New client connected with socket FD: " << client_sock << endl;

            // Handle each client in a new thread
            lock_guard<mutex> lock(data_mutex);
            client_threads[client_sock] = thread(process_quic_client, client_sock);
            client_threads[client_sock].detach();  // Run thread independently
        }

        close(server_sock);
    } catch (const exception& e) {
        cerr << "Error in telemetry handler: " << e.what() << endl;
    }
}
void handle_telemetry(int port) {
    try {
        int server_sock;
        struct sockaddr_in server_addr, client_addr;

        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
            cerr << "Error creating TCP socket: " << strerror(errno) << endl;
            return;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Binding error: " << strerror(errno) << endl;
            close(server_sock);
            return;
        }

        if (listen(server_sock, 5) < 0) {
            cerr << "Listening error: " << strerror(errno) << endl;
            close(server_sock);
            return;
        }
        // cout<<1111<<endl;
        cout << "TCP Server running on port " << port << endl;
        map<int, thread> client_threads;
        socklen_t addr_len = sizeof(client_addr);

        while (true) {
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock < 0) {
                cerr << "Error accepting connection: " << strerror(errno) << endl;
                continue;
            }

            cout << "New client connected with socket FD: " << client_sock << endl;

            // Handle each client in a new thread
            lock_guard<mutex> lock(data_mutex);
            client_threads[client_sock] = thread(process_tcp_client, client_sock);
            client_threads[client_sock].detach();  // Run thread independently
        }

        close(server_sock);
    } catch (const exception& e) {
        cerr << "Error in telemetry handler: " << e.what() << endl;
    }
}

void receive_udp_data(int udp_sock) {
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        int received_len = recvfrom(udp_sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (received_len < 0) {
            cerr << "Error receiving UDP data: " << strerror(errno) << endl;
        } else {
            buffer[received_len] = '\0';  // Null-terminate received data
            cout << "Received client ID: " << buffer << endl;
            lock_guard<mutex> lock(data_mutex);
            string client_id(buffer);
            client_map[client_id] = client_addr;
        }
    }
}

void send_udp_data(int udp_sock) {
    string command;
    string target_client;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        cout << "Enter client ID to send to: ";
        getline(cin, target_client);  // User input

        lock_guard<mutex> lock(data_mutex);
        auto it = client_map.find(target_client);
        if (it != client_map.end()) {
            client_addr = it->second;
            cout << "Enter command: ";
            getline(cin, command);

            // Send command to client
            int send_result = sendto(udp_sock, command.c_str(), command.size(), 0, (struct sockaddr*)&client_addr, client_addr_len);
            if (send_result < 0) {
                cerr << "Error sending UDP data: " << strerror(errno) << endl;
            } else {
                cout << "Command sent to client: " << command << endl;
            }
        } else {
            cerr << "Client ID not found." << endl;
        }
    }
}

void handle_udp(int port) {
    int udp_sock;
    struct sockaddr_in server_addr, client_addr;

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        cerr << "Error creating UDP socket: " << strerror(errno) << endl;
        return;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(udp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Binding error: " << strerror(errno) << endl;
        close(udp_sock);
        return;
    }

    cout << "UDP Server running on port " << port << endl;

    // Start receive and send threads
    thread receive_thread(receive_udp_data, udp_sock);
    thread send_thread(send_udp_data, udp_sock);

    // Wait for threads to complete (they run indefinitely)
    receive_thread.join();
    send_thread.join();

    // Close the socket
    close(udp_sock);
}

int main() {
    try {
        thread control_thread(handle_udp, control_port);
        thread telemetry_thread(handle_telemetry, telemetry_port);
        thread file_thread(handle_file, file_port);
        control_thread.join();
        telemetry_thread.join();
        file_thread.join();
    } catch (const exception& e) {
        cerr << "Main function error: " << e.what() << endl;
    }

    return 0;
}
