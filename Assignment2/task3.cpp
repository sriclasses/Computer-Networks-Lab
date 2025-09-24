// #include <iostream>
// #include <cstring>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <string>
// #include <thread>
// #include <map>
// #include <mutex>
// #include <fstream>
// #include <sstream>

// using namespace std;

// const int PORT = 8080;
// const string TARGET_DOMAIN = "www.google.com";
// const string TARGET_PORT = "80";

// // Function declarations
// void handleClient(int client_socket);
// void displayAnalytics(); // Function to display analytics
// void logRequest(const string& request, const string& client_ip);
// void logResponse(const string& response);
// string parseCookies(const string& response);
// void forwardCookies(int client_socket, const string& request);

// // Global data structures for cookie management, logging, and analytics
// map<int, string> cookieCache;  // Map client socket to cookies
// map<string, int> domainAccessCount; // Access count for domains
// mutex mtx;  // Mutex for thread safety

// ofstream logFile("log.txt", ios::app); // Open log file in append mode

// int main() {
//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     // Create socket
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         cerr << "Socket creation failed\n";
//         return -1;
//     }

//     // Attach socket to the port
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         cerr << "Set socket options failed\n";
//         return -1;
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     // Bind the socket to the port
//     if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
//         cerr << "Bind failed\n";
//         return -1;
//     }

//     // Start listening for connections
//     if (listen(server_fd, 3) < 0) {
//         cerr << "Listen failed\n";
//         return -1;
//     }

//     cout << "Proxy server is running on port " << PORT << "\n";

//     while (true) {
//         // Accept a new client connection
//         if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
//             cerr << "Accept failed\n";
//             return -1;
//         }

//         // Handle client requests in a new thread
//         thread(handleClient, new_socket).detach();
//     }

//     close(server_fd); // Close the server socket (not reached in this infinite loop)
//     logFile.close(); // Close the log file (not reached in this infinite loop)
//     return 0;
// }

// void handleClient(int client_socket) {
//     char buffer[4096] = {0};
//     int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     buffer[bytes_read] = '\0';

//     // Log the incoming request
//     string client_ip = inet_ntoa(((struct sockaddr_in*)&client_socket)->sin_addr);
//     logRequest(buffer, client_ip);

//     // Check if the request is a GET request
//     if (strncmp(buffer, "GET", 3) == 0) {
//         string request(buffer);

//         // Forward the request to the target server
//         struct sockaddr_in target_server;
//         target_server.sin_family = AF_INET;
//         target_server.sin_port = htons(stoi(TARGET_PORT));

//         // Convert domain to IP (for simplicity, this is hard-coded)
//         inet_pton(AF_INET, "142.250.195.36", &target_server.sin_addr); // Example IP for www.google.com

//         int server_socket = socket(AF_INET, SOCK_STREAM, 0);
//         connect(server_socket, (struct sockaddr*)&target_server, sizeof(target_server));

//         // Forward cookies, if any
//         forwardCookies(client_socket, request);
//         send(server_socket, request.c_str(), request.length(), 0);

//         // Receive the response
//         string response;
//         while ((bytes_read = read(server_socket, buffer, sizeof(buffer) - 1)) > 0) {
//             buffer[bytes_read] = '\0';
//             response += buffer;
//         }

//         // Log the response
//         logResponse(response);

//         // Parse and store cookies for www.google.com
//         if (request.find(TARGET_DOMAIN) != string::npos) {
//             string cookie = parseCookies(response);
//             if (!cookie.empty()) {
//                 mtx.lock();
//                 cookieCache[client_socket] = cookie;  // Store cookie for this client
//                 mtx.unlock();
//             }
//         }

//         // Send the response back to the client
//         send(client_socket, response.c_str(), response.length(), 0);
//         close(server_socket);
//     }

//     close(client_socket); // Close the client socket after serving
// }

// void logRequest(const string& request, const string& client_ip) {
//     string logEntry = "[LOG] Request from " + client_ip + ": " + request + "\n";
//     cout << logEntry;

//     // Write log entry to file
//     mtx.lock();
//     logFile << logEntry;
//     logFile.flush(); // Ensure that the log entry is written immediately
//     mtx.unlock();

//     // Update access count for the domain
//     size_t start = request.find("Host: ") + 6;
//     size_t end = request.find("\r\n", start);
//     string domain = request.substr(start, end - start);
//     mtx.lock();
//     domainAccessCount[domain]++;
//     mtx.unlock();
// }

// void logResponse(const string& response) {
//     string logEntry;
//     try {
//         logEntry = "[LOG] Response: " + response.substr(0, 1000) + "...\n"; // Log the first 1000 characters
//         cout << logEntry;
//     } catch (const char* msg) {
//         logEntry = string(msg) + "\n";
//         cout << logEntry;
//     }

//     // Write log entry to file
//     mtx.lock();
//     logFile << logEntry;
//     logFile.flush(); // Ensure that the log entry is written immediately
//     mtx.unlock();
// }

// string parseCookies(const string& response) {
//     string cookie;
//     size_t pos = response.find("Set-Cookie: ");
//     if (pos != string::npos) {
//         size_t start = pos + 12; // 12 for "Set-Cookie: "
//         size_t end = response.find("\r\n", start);
//         cookie = response.substr(start, end - start);
//     }
//     return cookie;
// }

// void forwardCookies(int client_socket, const string& request) {
//     mtx.lock();
//     if (cookieCache.find(client_socket) != cookieCache.end()) {
//         // If there are cookies stored for this client, add them to the request
//         string cookie = cookieCache[client_socket];
//         string new_request = request + "Cookie: " + cookie + "\r\n";
//         send(client_socket, new_request.c_str(), new_request.length(), 0);
//     }
//     mtx.unlock();
// }
// #include <iostream>
// #include <cstring>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <string>
// #include <thread>
// #include <map>
// #include <mutex>
// #include <sstream>

// using namespace std;

// const int PORT = 8080;
// const string TARGET_DOMAIN = "www.google.com";
// const string TARGET_PORT = "80";

// // Function declarations
// void handleClient(int client_socket);
// void displayAnalytics(); // Function to display analytics
// void logRequest(const string& request, const string& client_ip);
// void logResponse(const string& response);
// string parseCookies(const string& response);
// void forwardCookies(int client_socket, const string& request);

// // Global data structures for cookie management, logging, and analytics
// map<int, string> cookieCache;  // Map client socket to cookies
// map<string, int> domainAccessCount; // Access count for domains
// mutex mtx;  // Mutex for thread safety

// int main() {
//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     // Create socket
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         cerr << "Socket creation failed\n";
//         return -1;
//     }

//     // Attach socket to the port
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         cerr << "Set socket options failed\n";
//         return -1;
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     // Bind the socket to the port
//     if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
//         cerr << "Bind failed\n";
//         return -1;
//     }

//     // Start listening for connections
//     if (listen(server_fd, 3) < 0) {
//         cerr << "Listen failed\n";
//         return -1;
//     }

//     cout << "Proxy server is running on port " << PORT << "\n";

//     while (true) {
//         // Accept a new client connection
//         if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
//             cerr << "Accept failed\n";
//             return -1;
//         }

//         // Handle client requests in a new thread
//         thread(handleClient, new_socket).detach();
//     }

//     close(server_fd); // Close the server socket (not reached in this infinite loop)
//     return 0;
// }

// void handleClient(int client_socket) {
//     char buffer[4096] = {0};
//     int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     buffer[bytes_read] = '\0';

//     // Log the incoming request
//     string client_ip = inet_ntoa(((struct sockaddr_in*)&client_socket)->sin_addr);
//     logRequest(buffer, client_ip);

//     // Check if the request is a GET request
//     if (strncmp(buffer, "GET", 3) == 0) {
//         string request(buffer);

//         // Forward the request to the target server
//         struct sockaddr_in target_server;
//         target_server.sin_family = AF_INET;
//         target_server.sin_port = htons(stoi(TARGET_PORT));

//         // Convert domain to IP (for simplicity, this is hard-coded)
//         inet_pton(AF_INET, "142.250.195.36", &target_server.sin_addr); // Example IP for www.google.com

//         int server_socket = socket(AF_INET, SOCK_STREAM, 0);
//         connect(server_socket, (struct sockaddr*)&target_server, sizeof(target_server));

//         // Forward cookies, if any
//         forwardCookies(client_socket, request);
//         send(server_socket, request.c_str(), request.length(), 0);

//         // Receive the response
//         string response;
//         while ((bytes_read = read(server_socket, buffer, sizeof(buffer) - 1)) > 0) {
//             buffer[bytes_read] = '\0';
//             response += buffer;
//         }

//         // Log the response
//         logResponse(response);

//         // Parse and store cookies for www.google.com
//         if (request.find(TARGET_DOMAIN) != string::npos) {
//             string cookie = parseCookies(response);
//             if (!cookie.empty()) {
//                 mtx.lock();
//                 cookieCache[client_socket] = cookie;  // Store cookie for this client
//                 mtx.unlock();
//             }
//         }

//         // Send the response back to the client
//         send(client_socket, response.c_str(), response.length(), 0);
//         close(server_socket);
//     }

//     close(client_socket); // Close the client socket after serving
// }

// void logRequest(const string& request, const string& client_ip) {
//     cout << "[LOG] Request from " << client_ip << ": " << request << "\n";

//     // Update access count for the domain
//     size_t start = request.find("Host: ") + 6;
//     size_t end = request.find("\r\n", start);
//     string domain = request.substr(start, end - start);
//     mtx.lock();
//     domainAccessCount[domain]++;
//     mtx.unlock();
// }

// void logResponse(const string& response) {
//     try{
//         cout << "[LOG] Response: " << response.substr(0, 1000) << "...\n"; // Log the first 100 characters
//     }catch (const char* msg) {
//         cout << msg << endl;
//     }
// }

// string parseCookies(const string& response) {
//     string cookie;
//     size_t pos = response.find("Set-Cookie: ");
//     if (pos != string::npos) {
//         size_t start = pos + 12; // 12 for "Set-Cookie: "
//         size_t end = response.find("\r\n", start);
//         cookie = response.substr(start, end - start);
//     }
//     return cookie;
// }

// void forwardCookies(int client_socket, const string& request) {
//     mtx.lock();
//     if (cookieCache.find(client_socket) != cookieCache.end()) {
//         // If there are cookies stored for this client, add them to the request
//         string cookie = cookieCache[client_socket];
//         string new_request = request + "Cookie: " + cookie + "\r\n";
//         send(client_socket, new_request.c_str(), new_request.length(), 0);
//     }
//     mtx.unlock();
// }
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>

using namespace std;

const int PORT = 8080;
const string TARGET_DOMAIN = "www.google.com";
const string TARGET_PORT = "80";

// Function declarations
void handleClient(int client_socket);
void displayAnalytics(); // Function to display analytics
void logRequest(const string& request, const string& client_ip);
void logResponse(const string& response);
string parseCookies(const string& response);
void forwardCookies(int client_socket, const string& request);

// Global data structures for cookie management, logging, and analytics
map<int, string> cookieCache;  // Map client socket to cookies
map<string, int> domainAccessCount; // Access count for domains
mutex mtx;  // Mutex for thread safety

ofstream logFile("log.txt", ios::app); // Open log file in append mode


void writeMapToFile(const string& filename){
    // Open the file in output mode (truncates the file to overwrite it)
    ofstream file(filename, ios::out | ios::trunc);

    // Check if the file opened successfully
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return;
    }

    // Write each key-value pair to the file
    for (const auto& entry : domainAccessCount) {
        file << entry.first << ": " << entry.second << endl;
    }
    // Close the file
    file.close();
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Socket creation failed\n";
        return -1;
    }

    // Attach socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        cerr << "Set socket options failed\n";
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed\n";
        return -1;
    }

    // Start listening for connections
    if (listen(server_fd, 3) < 0) {
        cerr << "Listen failed\n";
        return -1;
    }

    cout << "Proxy server is running on port " << PORT << "\n";

    while (true) {
        // Accept a new client connection
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            cerr << "Accept failed\n";
            return -1;
        }

        // Handle client requests in a new thread
        thread(handleClient, new_socket).detach();
    }

    close(server_fd); // Close the server socket (not reached in this infinite loop)
    logFile.close(); // Close the log file (not reached in this infinite loop)
    return 0;
}

void handleClient(int client_socket) {
    char buffer[4096] = {0};
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';

    // Log the incoming request
    string client_ip = inet_ntoa(((struct sockaddr_in*)&client_socket)->sin_addr);
    logRequest(buffer, client_ip);

    // Check if the request is a GET request
    if (strncmp(buffer, "GET", 3) == 0) {
        string request(buffer);

        // Forward the request to the target server
        struct sockaddr_in target_server;
        target_server.sin_family = AF_INET;
        target_server.sin_port = htons(stoi(TARGET_PORT));

        // Convert domain to IP (for simplicity, this is hard-coded)
        inet_pton(AF_INET, "142.250.195.36", &target_server.sin_addr); // Example IP for www.google.com

        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        connect(server_socket, (struct sockaddr*)&target_server, sizeof(target_server));

        // Forward cookies, if any
        forwardCookies(client_socket, request);
        send(server_socket, request.c_str(), request.length(), 0);

        // Receive the response
        string response;
        while ((bytes_read = read(server_socket, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            response += buffer;
        }

        // Log the response
        logResponse(response);

        // Parse and store cookies for www.google.com
        if (request.find(TARGET_DOMAIN) != string::npos) {
            string cookie = parseCookies(response);
            if (!cookie.empty()) {
                mtx.lock();
                cookieCache[client_socket] = cookie;  // Store cookie for this client
                mtx.unlock();
            }
        }

        // Send the response back to the client
        send(client_socket, response.c_str(), response.length(), 0);
        close(server_socket);
    }
    cookieCache.erase(client_socket);
    close(client_socket); // Close the client socket after serving
}

void logRequest(const string& request, const string& client_ip) {
    string logEntry = "[LOG] Request from " + client_ip + ": " + request + "\n";
    cout << logEntry;

    // Write log entry to file
    mtx.lock();
    logFile << logEntry;
    logFile.flush(); // Ensure that the log entry is written immediately
    mtx.unlock();

    // Update access count for the domain
    size_t start = request.find("Host: ") + 6;
    size_t end = request.find("\r\n", start);
    string domain = request.substr(start, end - start);
    mtx.lock();
    domainAccessCount[domain]++;
    writeMapToFile("Analytics.txt"  );
    mtx.unlock();
}

void logResponse(const string& response) {
    string logEntry;
    try {
        logEntry = "[LOG] Response: " + response.substr(0, 1000) + "...\n"; // Log the first 1000 characters
        cout << logEntry;
    } catch (const char* msg) {
        logEntry = string(msg) + "\n";
        cout << logEntry;
    }

    // Write log entry to file
    mtx.lock();
    logFile << logEntry;
    logFile.flush(); // Ensure that the log entry is written immediately
    mtx.unlock();
}

string parseCookies(const string& response) {
    string cookie;
    size_t pos = response.find("Set-Cookie: ");
    if (pos != string::npos) {
        size_t start = pos + 12; // 12 for "Set-Cookie: "
        size_t end = response.find("\r\n", start);
        cookie = response.substr(start, end - start);
    }
    return cookie;
}

void forwardCookies(int client_socket, const string& request) {
    mtx.lock();
    if (cookieCache.find(client_socket) != cookieCache.end()) {
        // If there are cookies stored for this client, add them to the request
        string cookie = cookieCache[client_socket];
        string new_request = request + "Cookie: " + cookie + "\r\n";
        send(client_socket, new_request.c_str(), new_request.length(), 0);
    }
    mtx.unlock();
}