# Makefile for Drone Communication System (Task 1) and Weather Monitoring System (Task 2)

# Compiler and flags
CXX = g++
CXXFLAGS = -pthread -lz

# Task 1: Drone Communication System
DRONE_SERVER = drone_server
DRONE_CLIENT = drone_client
DRONE_SRCS = server.cpp client.cpp

# Task 2: Weather Monitoring System
WEATHER_SERVER = weather_server
WEATHER_CLIENT = weather_client
WEATHER_SRCS = server.cpp client.cpp

# Default target: Build both tasks
all: build_task1 build_task2

# Build Task 1: Drone Communication System
build_task1: $(DRONE_SERVER) $(DRONE_CLIENT)

$(DRONE_SERVER): server.cpp
	$(CXX) $(CXXFLAGS) -o $@ server.cpp

$(DRONE_CLIENT): client.cpp
	$(CXX) $(CXXFLAGS) -o $@ client.cpp

# Build Task 2: Weather Monitoring System
build_task2: $(WEATHER_SERVER) $(WEATHER_CLIENT)

$(WEATHER_SERVER): server.cpp
	$(CXX) $(CXXFLAGS) -o $@ server.cpp

$(WEATHER_CLIENT): client.cpp
	$(CXX) $(CXXFLAGS) -o $@ client.cpp

# Clean the build
clean:
	rm -f $(DRONE_SERVER) $(DRONE_CLIENT) $(WEATHER_SERVER) $(WEATHER_CLIENT)
	rm -f server_output.log
