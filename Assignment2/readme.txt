Project: Network Communication and HTTP Proxy Server
Overview
This project consists of three main tasks that revolve around analyzing HTTP traffic, simulating DNS lookups, and implementing an HTTP Proxy Server with caching and cookie management functionalities.


Dependencies

Task 1: HTTP using Wireshark
Task 2: Web Server, HTTP Proxy with Caching, and DNS Lookup Simulation
Task 3: Develop an HTTP Proxy Server

Ensure that your environment meets the following requirements:

Wireshark: For HTTP traffic capture and analysis in Task 1.
GCC/G++: For compiling the C++ programs.
pcap file: For working with network traffic capture files.




Task 1: HTTP using Wireshark
Objective: Capture and analyze HTTP traffic using Wireshark.

Steps:
Capture HTTP traffic and identify GET requests and their corresponding responses.
Analyze HTTP request/response headers and calculate response times.
Inspect the content type of the responses and extract relevant data.




Task 2: Web Server, HTTP Proxy with Caching, and DNS Lookup Simulation
Objective: Simulate DNS lookups, implement an HTTP Proxy Server with caching, and analyze their performance.

Steps:
Simulate DNS lookups with delays/failures.
Implement a simplified HTTP Proxy Server with caching using sockets.
Implement a Least Recently Used (LRU) cache mechanism for web pages.




Task 3: Develop an HTTP Proxy Server
Objective: Develop an HTTP Proxy Server in C++ that handles client requests, manages cookies, and provides logging and analytics.

Steps:
Implement an HTTP Proxy Server that forwards client requests to the appropriate servers.
Parse, store, and forward cookies specifically for www.amazon.com.
Implement session management and logging of HTTP requests/responses.
Provide analytics on frequently accessed domains and cookies.

1. Compiler:
g++ (GNU C++ Compiler): Required to compile your C++ source files (task2.cpp and task3.cpp). Ensure it's installed on your system.

2. System Libraries:
sys/socket.h: This header file provides the socket programming interface for creating and managing network connections.
arpa/inet.h: This header is used for various internet operations, such as IP address manipulation, needed for network programming.

3. POSIX Threads Library (-lpthread):
Purpose: Necessary for multithreading, which is commonly required in networked applications to handle multiple connections simultaneously.

4. C++ Standard Library:
-std=c++11: Ensures that the code is compiled using the C++11 standard, providing modern C++ features.

5. Network Operations with curl:
curl Command:
The command curl -x http://localhost:8080 -v http://www.google.com is used to send an HTTP request to www.google.com through a proxy server running on localhost at port 8080.
-x http://localhost:8080: Specifies the proxy server to use for the request.
-v: Enables verbose mode, providing detailed information about the request/response process.


Summary
This project covers the analysis of HTTP traffic, the intricacies of DNS lookups, and the development of an HTTP Proxy Server with caching and cookie management functionalities. Each task is aimed at providing hands-on experience with network protocols, DNS processes, and proxy server mechanisms.

