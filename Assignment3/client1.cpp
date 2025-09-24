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
#include <chrono>
#include <mutex>
#include <sstream>
#include <fstream>
using namespace std;

mutex movement_mutex;

#define BUFFER_SIZE 1024
#define FILE_TRANSFER_PORT 9002
struct Position {
    int x;
    int y;
    int speed;
    bool is_active;
    string direction;
    Position() : x(0), y(0), speed(0), is_active(false), direction("None") {};
};
char encryption_key = 'K'; // XOR key for encryption/decryption

// Thread function to update movement based on direction and speed
void update_position(Position &pos) {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(1000)); // Update every 1 second

        lock_guard<mutex> lock(movement_mutex);
        if (pos.is_active) {
            if (pos.direction == "right") {
                pos.x += pos.speed;
            } else if (pos.direction == "left") {
                pos.x -= pos.speed;
            } else if (pos.direction == "up") {
                pos.y += pos.speed;
            } else if (pos.direction == "down") {
                pos.y -= pos.speed;
            }
            cout << "Moving " << pos.direction << " to (" << pos.x << ", " << pos.y << ") at speed " << pos.speed << endl;
        }
    }
}
void QUIChelperfunction(int &client_fd,sockaddr_in& serverAddr){
    // Configure the server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(FILE_TRANSFER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Localhost or replace with server IP

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed: " << strerror(errno) << std::endl;
        close(client_fd);
        exit(EXIT_FAILURE);
    }
}

vector<char> xor_encrypt(const vector<char>& data, char key) {
    vector<char> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ key;
    }
    return result;
}


string convert_to_string(const Position& pos) {
    stringstream ss;
    ss << pos.x << " " << pos.y << " " << pos.speed << " " << pos.is_active << " " << pos.direction;
    return ss.str();
}

void send_position_data(const string& server_ip, int port, const string& client_id, Position &pos) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Socket creation failed: " << strerror(errno) << endl;
        return;
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection failed: " << strerror(errno) << endl;
        close(sockfd);
        return;
    }

    cout << "Connected to server" << endl;
    // Send client ID
    ssize_t id_bytes_sent = send(sockfd, client_id.c_str(), client_id.size(), 0);
    if (id_bytes_sent < 0) {
        cerr << "Failed to send client ID: " << strerror(errno) << endl;
        close(sockfd);
        return;
    }
    sleep(1);
    
    while (true) {
        // Encrypt position data
        string serialized_data = convert_to_string(pos);
        auto encrypted_data = xor_encrypt(vector<char>(serialized_data.begin(), serialized_data.end()), encryption_key);

        // Send encrypted position data
        ssize_t bytes_sent = send(sockfd, encrypted_data.data(), encrypted_data.size(), 0);
        if (bytes_sent < 0) {
            cerr << "Failed to send data: " << strerror(errno) << endl;
        } else {
            cout << "Sent " << bytes_sent << " bytes." << endl;
        }

        // Optional delay to manage data flow
        sleep(10);
    }

    // Close the socket
    close(sockfd);
}
std::string xorEncryptDecrypt(const std::string &data, char key) {
    std::string result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= key;
    }
    return result;
}



void QUICFileTransferClient(string filename) {
    int client_fd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE] = {0};
    const std::string fileName = "log.txt";
    // string file = "log.txt" ; 

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    QUIChelperfunction(client_fd,serverAddr);

    // Open the file to read its content
    ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << fileName << std::endl;
        close(client_fd);
        return;
    }

    // Send the file in chunks
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        std::streamsize bytesRead = file.gcount();

        // Encrypt the chunk before sending
        std::string encryptedChunk = xorEncryptDecrypt(std::string(buffer, bytesRead), 'K');

        // Send the encrypted chunk
        if (send(client_fd, encryptedChunk.c_str(), encryptedChunk.size(), 0) < 0) {
            std::cerr << "Error sending file: " << strerror(errno) << std::endl;
            break;
        }
    }

    std::cout << "File sent successfully." << std::endl;

    // Close file and clean up
    file.close();
    close(client_fd);
}

// Function to handle commands and adjust position accordingly
void handle_command(const string& cmd, Position& pos) {
    lock_guard<mutex> lock(movement_mutex);

    if (cmd.rfind("Start", 0) == 0) {  // Start command with speed
        int speed = stoi(cmd.substr(6));  // Extract speed
        pos.speed = speed;
        pos.is_active = true;
        pos.direction = "right";  // Default direction
        cout << "Started with speed " << pos.speed << endl;

    } else if (cmd == "move left") {
        pos.direction = "left";
        cout << "Direction changed to left" << endl;

    } else if (cmd == "move right") {
        pos.direction = "right";
        cout << "Direction changed to right" << endl;

    } else if (cmd == "move up") {
        pos.direction = "up";
        cout << "Direction changed to up" << endl;

    } else if (cmd == "move down") {
        pos.direction = "down";
        cout << "Direction changed to down" << endl;

    } else if (cmd.rfind("inc", 0) == 0) {  // Increase speed
        int increment = stoi(cmd.substr(4));  // Extract increment
        pos.speed += increment;
        cout << "Speed increased by " << increment << ", new speed: " << pos.speed << endl;

    } else if (cmd.rfind("dec", 0) == 0) {  // Decrease speed
        int decrement = stoi(cmd.substr(4));  // Extract decrement
        pos.speed = max(0, pos.speed - decrement);  // Prevent negative speed
        cout << "Speed decreased by " << decrement << ", new speed: " << pos.speed << endl;

    } else if (cmd == "stop") {  // Stop movement
        pos.speed = 0;
        pos.is_active = false;
        cout << "Movement stopped" << endl;
    } 
    else if (cmd.rfind("send", 0) == 0 ){
        string filename = cmd.substr(5) ; 
        thread file_call(QUICFileTransferClient , filename );
        file_call.detach();
    }
}

void send_udp_commands(const string& server_ip, int port, const string& client_id, Position &pos) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        cerr << "Socket creation failed: " << strerror(errno) << endl;
        return;
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    // Send client ID
    int send_result = sendto(sockfd, client_id.c_str(), client_id.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (send_result < 0) {
        cerr << "Failed to send client ID: " << strerror(errno) << endl;
        close(sockfd);
        return;
    }

    cout << "Client ID sent: " << client_id << endl;
    sleep(1);

    // Receive commands from server
    char buffer[BUFFER_SIZE] = {0};
    socklen_t server_addr_len = sizeof(server_addr);
    while (true) {
        int recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &server_addr_len);
        if (recv_len < 0) {
            cerr << "Receive failed: " << strerror(errno) << endl;
        } else {
            buffer[recv_len] = '\0';  // Null-terminate data
            cout << "Received command: " << buffer << endl;
            string command(buffer);
            handle_command(command, pos);
        }
    }

    // Close the socket
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <client_id>" << endl;
        return 1;
    }
    string server_ip = "127.0.0.1"; // Server IP
    // int control_port = 8080;
    // int telemetry_port = 8081;
    // int file_port = 8082 ; 
    int control_port = 9000;
    int telemetry_port = 9001;
    int file_port = 9002; 
    string client_id = argv[1];
    
    Position pos;
    thread movement_thread(update_position, ref(pos));
    thread udp_thread(send_udp_commands, server_ip, control_port, client_id, ref(pos));
    thread telemetry_thread(send_position_data, server_ip, telemetry_port, client_id, ref(pos));
    // thread file_call(QUICFileTransferClient );
    udp_thread.join();
    telemetry_thread.join();
    
    return 0;
}
