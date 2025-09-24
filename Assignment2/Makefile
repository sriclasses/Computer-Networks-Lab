# makefile

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -g -std=c++11

# Libraries needed for socket programming
LIBS = -lpthread

# Source files
SRCS = task2.cpp task3.cpp

# Executable names
TARGETS = task2 task3

# Default target to build both executables
all: $(TARGETS)

# Build task2
task2: task2.cpp
	$(CXX) $(CXXFLAGS) task2.cpp -o task2 $(LIBS)

# Build task3
task3: task3.cpp
	$(CXX) $(CXXFLAGS) task3.cpp -o task3 $(LIBS)

# Clean up the compiled files
clean:
	rm -f $(TARGETS)