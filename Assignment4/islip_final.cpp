#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <queue>
#include <csignal>

#define NUM_INPUT_PORTS 8
#define NUM_OUTPUT_PORTS 8
#define BUFFER_SIZE 64
#define SIMULATION_TIME 1500       // Total simulation time in milliseconds
#define TIME_SLICE 50              // Time allocated for processing each input queue in milliseconds
#define BASE_PACKET_ARRIVAL_RATE 2 // Base rate of packet arrival per time slice

using namespace std;

// Packet structure for storing packet information.
struct Packet {
    int id;              // Unique ID for the packet
    int arrival_time;    // Time when packet arrives
    int service_time;    // Time needed to process packet at output
    int waiting_time;    // Time packet spends waiting in the queue
    int output_port;     // Destination port for the packet
};

// Virtual Output Queue (VOQ) structure for each input port with a queue for each output port.
struct VOQ {
    queue<Packet> packets[NUM_OUTPUT_PORTS];
    int drop_count = 0;  // Count of dropped packets for this VOQ
};

// Global variables for simulation metrics and synchronization
VOQ input_queues[NUM_INPUT_PORTS];             // Input queues for packets
queue<Packet> output_queues[NUM_OUTPUT_PORTS]; // Output queues for processing packets
int total_packets_sent = 0;                    // Total packets generated
int total_packets_processed = 0;               // Total packets successfully processed
int total_waiting_time = 0;                    // Total waiting time for all packets
int total_turnaround_time = 0;                 // Total turnaround time for all packets
int total_dropped_packets = 0;                 // Total packets dropped due to buffer overflow
double total_buffer_occupancy = 0;             // Aggregate buffer occupancy over time
int current_time = 0;                          // Tracks simulation time
bool stop_simulation = false;                  // Flag to stop simulation when triggered

mutex input_mutex[NUM_INPUT_PORTS];            // Mutex for each input queue
mutex output_mutex[NUM_OUTPUT_PORTS];          // Mutex for each output queue
mutex switch_mutex;                            // Mutex for scheduling process

// Function to generate packets at each input port based on traffic type.
void generate_packets(int port, const string& traffic_type) {
    default_random_engine generator;
    uniform_int_distribution<int> distribution(1, 100);

    while (current_time < SIMULATION_TIME) {
        this_thread::sleep_for(chrono::milliseconds(TIME_SLICE));
        lock_guard<mutex> lock(input_mutex[port]);

        int arrival_rate = BASE_PACKET_ARRIVAL_RATE;

        // Adjust packet arrival rate based on traffic pattern
        if (traffic_type == "Non-Uniform") {
            arrival_rate += (port % 2 == 0) ? 1 : -1;
        } else if (traffic_type == "Bursty" && rand() % 100 < 10) {
            arrival_rate += 3;
        }

        // Generate packets based on arrival rate and attempt to enqueue them.
        for (int i = 0; i < arrival_rate; ++i) {
            Packet p;
            p.id = total_packets_sent + 1;
            p.arrival_time = current_time;
            p.service_time = rand() % 5 + 1;  // Random service time between 1 and 5 ms
            p.output_port = rand() % NUM_OUTPUT_PORTS;
            p.waiting_time = 0;

            // Check if buffer has space for the new packet
            if (input_queues[port].packets[p.output_port].size() < BUFFER_SIZE) {
                input_queues[port].packets[p.output_port].push(p);
                total_packets_sent++;
            } else {
                input_queues[port].drop_count++;
                total_dropped_packets++;
            }
        }

        // Update total buffer occupancy
        for (int i = 0; i < NUM_OUTPUT_PORTS; ++i) {
            total_buffer_occupancy += input_queues[port].packets[i].size();
        }
    }
}

// Function to schedule packets from input to output queues using the iSLIP algorithm.
void iSLIP_scheduler() {
    int input_rr[NUM_INPUT_PORTS] = {0};   // Round-robin pointer for inputs
    int output_rr[NUM_OUTPUT_PORTS] = {0}; // Round-robin pointer for outputs

    while (current_time < SIMULATION_TIME) {
        current_time += TIME_SLICE;
        this_thread::sleep_for(chrono::milliseconds(TIME_SLICE));
        lock_guard<mutex> lock(switch_mutex);

        cout << "\n=== Request Phase ===\n";
        vector<int> requests(NUM_OUTPUT_PORTS, -1);

        // Request Phase: Each input requests access to an output port if it has packets for it.
        for (int i = 0; i < NUM_OUTPUT_PORTS; ++i) {
            for (int j = 0; j < NUM_INPUT_PORTS; ++j) {
                int input_port = (j + input_rr[j]) % NUM_INPUT_PORTS;
                if (!input_queues[input_port].packets[i].empty()) {
                    Packet& p = input_queues[input_port].packets[i].front();
                    requests[i] = input_port;
                    cout << "Input Port " << input_port << " requests Output Port " << i
                         << " for Packet ID " << p.id << "\n";
                }
            }
        }

        cout << "\n=== Grant Phase ===\n";
        vector<int> grants(NUM_INPUT_PORTS, -1);

        // Grant Phase: Output ports grant requests based on round-robin policy.
        for (int i = 0; i < NUM_OUTPUT_PORTS; ++i) {
            int granted_input = requests[i];
            if (granted_input != -1) {
                Packet& p = input_queues[granted_input].packets[i].front();
                grants[granted_input] = i;
                cout << "Output Port " << i << " grants request from Input Port "
                     << granted_input << " for Packet ID " << p.id << "\n";
            }
        }

        cout << "\n=== Accept Phase ===\n";

        // Accept Phase: Inputs accept grants and transfer packets to output queues.
        for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
            int accepted_output = grants[i];
            if (accepted_output != -1) {
                input_rr[i] = (input_rr[i] + 1) % NUM_INPUT_PORTS;
                output_rr[accepted_output] = (output_rr[accepted_output] + 1) % NUM_OUTPUT_PORTS;

                Packet p = input_queues[i].packets[accepted_output].front();
                input_queues[i].packets[accepted_output].pop();

                int waiting_time = current_time - p.arrival_time;
                p.waiting_time = waiting_time;
                total_waiting_time += waiting_time;
                total_turnaround_time += waiting_time + p.service_time;

                lock_guard<mutex> output_lock(output_mutex[accepted_output]);
                output_queues[accepted_output].push(p);
                total_packets_processed++;

                cout << "Input Port " << i << " accepts grant to Output Port "
                     << accepted_output << " for Packet ID " << p.id << "\n";
            }
        }
    }
}

// Function to simulate processing packets at output ports
void process_output_queue(int port) {
    while (current_time < SIMULATION_TIME) {
        this_thread::sleep_for(chrono::milliseconds(50));
        lock_guard<mutex> lock(output_mutex[port]);

        while (!output_queues[port].empty()) {
            Packet p = output_queues[port].front();
            output_queues[port].pop();
            this_thread::sleep_for(chrono::milliseconds(p.service_time));
            cout << "Processed Packet ID " << p.id << " at Output Port " << port << "\n";
        }
    }
}

// Function to calculate and display final metrics for the simulation
void calculate_metrics() {
    double throughput = static_cast<double>(total_packets_processed) / (SIMULATION_TIME / 1000.0);
    double avg_waiting_time = total_packets_processed ? static_cast<double>(total_waiting_time) / total_packets_processed : 0;
    double avg_turnaround_time = total_packets_processed ? static_cast<double>(total_turnaround_time) / total_packets_processed : 0;
    double avg_buffer_occupancy = total_buffer_occupancy / (NUM_INPUT_PORTS * SIMULATION_TIME / TIME_SLICE);

    cout << "\nMetrics:\n";
    cout << "Total Packets Sent: " << total_packets_processed + total_dropped_packets << "\n";
    cout << "Total Packets Dropped: " << total_dropped_packets << "\n";
    cout << "Total Packets Processed: " << total_packets_processed << "\n";
    cout << "Throughput: " << throughput << " packets/second\n";
    cout << "Average Waiting Time: " << avg_waiting_time << " ms\n";
    cout << "Average Turnaround Time: " << avg_turnaround_time << " ms\n";
    cout << "Average Buffer Occupancy: " << avg_buffer_occupancy << " packets\n";
}

int main() {
    vector<thread> threads;

    // Start packet generation threads for each input port with a specified traffic pattern
    for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
        threads.emplace_back(generate_packets, i, "Non-Uniform");
    }

    // Start iSLIP scheduler thread
    threads.emplace_back(iSLIP_scheduler);

    // Start processing threads for each output queue
    for (int i = 0; i < NUM_OUTPUT_PORTS; ++i) {
        threads.emplace_back(process_output_queue, i);
    }

    // Join all threads
    for (auto& t : threads) {
        t.join();
    }

    // Calculate and display metrics
    calculate_metrics();

    return 0;
}
