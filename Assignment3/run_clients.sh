#!/bin/bash

# Number of clients to spawn
CLIENT_COUNT=5

# Run multiple clients in parallel
for i in $(seq 1 $CLIENT_COUNT); do
./client > client_output_$i.log 2>&1 &
done