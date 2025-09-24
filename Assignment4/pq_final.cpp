#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <random>
#include <csignal>

#define NUM_INPUT_PORTS 8
#define NUM_OUTPUT_PORTS 8
#define BUFFER_SIZE 64
#define SIMULATION_TIME 500       // Total simulation time in milliseconds
#define TIME_SLICE 50              // Time allocated for processing each input queue in milliseconds
#define BASE_PACKET_ARRIVAL_RATE 1 // Base rate of packet arrival per time slice

using namespace std;

struct Packet
{
    int id;
    int arrival_time;
    int service_time;
    int waiting_time;
    int output_port;
    int priority; // Added priority for packets
};

struct Queue
{
    queue<Packet> packets;
    int drop_count = 0;
};

// Global variables
Queue input_queues[NUM_INPUT_PORTS];
Queue output_queues[NUM_OUTPUT_PORTS];
int total_packets_processed = 0;
int total_waiting_time = 0;
int total_turnaround_time = 0;
int total_packets = 0;
int current_time = 0;
bool stop_simulation = false;  // A flag to stop the simulation
int total_dropped_packets = 0; // Total dropped packets

mutex input_mutex[NUM_INPUT_PORTS];
mutex output_mutex[NUM_OUTPUT_PORTS];
mutex switch_mutex;

// Function to generate random traffic patterns
void generate_packets(int port, const string &traffic_type)
{
    default_random_engine generator;
    uniform_int_distribution<int> distribution(1, 100);

    while (current_time < SIMULATION_TIME)
    {
        this_thread::sleep_for(chrono::milliseconds(TIME_SLICE)); // Simulate time between packet generations
        lock_guard<mutex> lock(input_mutex[port]);

        int arrival_rate = BASE_PACKET_ARRIVAL_RATE; // Default arrival rate
        if (traffic_type == "Non-Uniform")
        {
            if (port % 2 == 0)
            {
                arrival_rate += 1; // Increase probability for even ports
            }
            else
            {
                arrival_rate = max(1, arrival_rate - 1); // Ensure arrival rate does not go below 1
            }
        }
        else if (traffic_type == "Bursty")
        {
            if (rand() % 100 < 10)
            {
                arrival_rate += 3; // Increase packet arrival rate in bursty traffic
            }
        }

        total_packets += arrival_rate;
        for (int i = 0; i < arrival_rate; ++i)
        {
            Packet p;
            p.id = total_packets_processed + 1;
            p.arrival_time = current_time;
            p.service_time = rand() % 5 + 5; // Random service time between 1 and 5
            p.output_port = rand() % NUM_OUTPUT_PORTS;
            p.waiting_time = 0;
            p.priority = rand() % 5 + 1; // Random priority between 1 and 5

            // Only add packet if the input queue is not full
            if (input_queues[port].packets.size() < BUFFER_SIZE)
            {
                input_queues[port].packets.push(p);
                total_packets_processed++;
            }
            else
            {
                input_queues[port].drop_count++;
                total_dropped_packets++; // Increment total dropped packets
            }
        }
    }
}

// Priority scheduling switch mechanism
void switch_mechanism()
{
    while (current_time < SIMULATION_TIME)
    {
        this_thread::sleep_for(chrono::milliseconds(TIME_SLICE)); // Simulate switch time

        lock_guard<mutex> lock(switch_mutex);

        // Find the highest priority packet across all input queues
        Packet highest_priority_packet;
        int input_port_with_highest_priority = -1; // Track from which port this packet came
        bool packet_found = false;

        // Check all input queues for the highest priority packet
        for (int i = 0; i < NUM_INPUT_PORTS; ++i)
        {
            lock_guard<mutex> lock(input_mutex[i]); // Lock the specific input mutex
            if (!input_queues[i].packets.empty())
            {
                Packet p = input_queues[i].packets.front();
                if (!packet_found || p.priority > highest_priority_packet.priority)
                {
                    highest_priority_packet = p;
                    input_port_with_highest_priority = i;
                    packet_found = true;
                }
            }
        }

        // Process the highest priority packet if found
        if (packet_found)
        {
            // Remove the packet from its input queue
            lock_guard<mutex> lock(input_mutex[input_port_with_highest_priority]);
            input_queues[input_port_with_highest_priority].packets.pop();

            // Increase waiting time for remaining packets in the input queue
            queue<Packet> temp_queue;
            while (!input_queues[input_port_with_highest_priority].packets.empty())
            {
                Packet next_packet = input_queues[input_port_with_highest_priority].packets.front();
                input_queues[input_port_with_highest_priority].packets.pop();
                next_packet.waiting_time += TIME_SLICE; // Increment waiting time
                temp_queue.push(next_packet);
            }
            input_queues[input_port_with_highest_priority].packets = temp_queue;

            lock_guard<mutex> output_lock(output_mutex[highest_priority_packet.output_port]);
            if (output_queues[highest_priority_packet.output_port].packets.size() < BUFFER_SIZE)
            {
                // Add packet to the output queue
                output_queues[highest_priority_packet.output_port].packets.push(highest_priority_packet);
                total_waiting_time += highest_priority_packet.waiting_time;
                total_turnaround_time += (highest_priority_packet.waiting_time + highest_priority_packet.service_time);

                cout << "Packet ID: " << highest_priority_packet.id 
                     << " moved from Input Port " << input_port_with_highest_priority 
                     << " to Output Port " << highest_priority_packet.output_port 
                     << " (Service Time: " << highest_priority_packet.service_time 
                     << "ms, Waiting Time: " << highest_priority_packet.waiting_time << "ms)" << endl;

                // Simulate processing time for the packet
                this_thread::sleep_for(chrono::milliseconds(highest_priority_packet.service_time));
            }
            else
            {
                // Increment drop count if output queue is full
                output_queues[highest_priority_packet.output_port].drop_count++;
                total_dropped_packets++; // Increment total dropped packets
                cout << "Packet ID: " << highest_priority_packet.id 
                     << " dropped due to output port " << highest_priority_packet.output_port 
                     << " being full." << endl;
            }
        }
    }
}

// Function to process packets in the output queues
void process_output_queue(int port)
{
    while (current_time < SIMULATION_TIME)
    {
        this_thread::sleep_for(chrono::milliseconds(1)); // Simulate output processing delay
        lock_guard<mutex> lock(output_mutex[port]);

        while (!output_queues[port].packets.empty())
        {
            Packet p = output_queues[port].packets.front();
            output_queues[port].packets.pop();
            this_thread::sleep_for(chrono::milliseconds(p.service_time)); // Simulate packet processing time
            cout << "Processed Packet ID: " << p.id << " from Output Port: " << port << endl;
        }
    }
}

// Function to calculate and print metrics
void calculate_metrics()
{
    double throughput = static_cast<double>(total_packets_processed) / (SIMULATION_TIME / 1000.0); // Packets per second
    double avg_waiting_time = total_packets_processed ? static_cast<double>(total_waiting_time) / total_packets_processed : 0;
    double avg_turnaround_time = total_packets_processed ? static_cast<double>(total_turnaround_time) / total_packets_processed : 0;

    // Calculate average buffer occupancy
    cout << "\nMetrics:\n";
    cout << "Total Packets Processed: " << total_packets_processed << "\n";
    cout << "Total Dropped Packets: " << total_dropped_packets << "\n";
    cout << "Throughput: " << throughput << " packets/second\n";
    cout << "Average Waiting Time: " << avg_waiting_time << " ms\n";
    cout << "Average Turnaround Time: " << avg_turnaround_time << " ms\n";
    
    float count = 0;
    for (int i = 0; i < NUM_OUTPUT_PORTS; i++)
    {
        count += output_queues[i].packets.size() + input_queues[i].packets.size();
    }
    cout << "Average Buffer Occupancy: " << count / float(16) << " packets\n";
}

void signalHandler(int signum)
{
    cout << "\nSimulation interrupted. Calculating metrics...\n";
    calculate_metrics();
    exit(signum);
}

int main()
{
    srand(static_cast<unsigned>(time(0))); // Seed random number generator

    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    // Choose traffic type
    string traffic_type = "Non-Uniform";

    // Create threads for input ports
    vector<thread> input_threads;
    for (int i = 0; i < NUM_INPUT_PORTS; ++i)
    {
        input_threads.push_back(thread(generate_packets, i, traffic_type));
    }

    // Create threads for output ports
    vector<thread> output_threads;
    for (int i = 0; i < NUM_OUTPUT_PORTS; ++i)
    {
        output_threads.push_back(thread(process_output_queue, i));
    }

    // Create the switch mechanism thread (round-robin)
    thread switch_thread(switch_mechanism);

    // Join all threads
    for (auto &t : input_threads)
    {
        t.join();
    }
    switch_thread.join();
    for (auto &t : output_threads)
    {
        t.join();
    }

    // Calculate and display the metrics
    calculate_metrics();

    return 0;
}

