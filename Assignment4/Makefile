# Makefile for Router Switch Fabric Simulation

# Compiler
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Source files
SRC = pq_final.cpp rr_final.cpp wfq_final.cpp islip_final.cpp

# Executable files
EXE = pq_sim rr_sim wfq_sim islip_sim

# Default target: compile all executables
all: $(EXE)

# Compile each simulation
pq_sim: pq_final.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

rr_sim: rr_final.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

wfq_sim: wfq_final.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

islip_sim: islip_final.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Run the simulations
run_pq: pq_sim
	./pq_sim

run_rr: rr_sim
	./rr_sim

run_wfq: wfq_sim
	./wfq_sim

run_islip: islip_sim
	./islip_sim

# Log iSLIP output to output.log
log_islip: islip_sim
	./islip_sim > output.log 2>&1

# Clean up build files
clean:
	rm -f $(EXE) output.log

.PHONY: all run_pq run_rr run_wfq run_islip log_islip clean
