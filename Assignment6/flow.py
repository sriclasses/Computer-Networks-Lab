import numpy as np
import matplotlib.pyplot as plt

# Load data from CSV using numpy
data = np.genfromtxt('network_metrics.csv', delimiter=',', skip_header=1)

# Extract columns for each metric
time = data[:, 0]
throughput = data[:, 3]
average_delay = data[:, 5]
packet_loss = data[:, 4]

# Create a figure and subplots for each metric
fig, axs = plt.subplots(3, 1, figsize=(10, 18))

# Plot throughput
axs[0].plot(time, throughput, marker='o', color='b', label="Throughput (Mbps)")
axs[0].set_xlabel("Time (s)")
axs[0].set_ylabel("Throughput (Mbps)")
axs[0].set_title("Throughput over Time")
axs[0].legend()
axs[0].grid(True)

# Plot average delay
axs[1].plot(time, average_delay, marker='o', color='g', label="Average Delay (s)")
axs[1].set_xlabel("Time (s)")
axs[1].set_ylabel("Average Delay (s)")
axs[1].set_title("Average Delay over Time")
axs[1].legend()
axs[1].grid(True)

# Plot packet loss ratio
axs[2].plot(time, packet_loss, marker='o', color='r', label="Packet Loss (%)")
axs[2].set_xlabel("Time (s)")
axs[2].set_ylabel("Packet Loss (%)")
axs[2].set_title("Packet Loss over Time")
axs[2].legend()
axs[2].grid(True)

# Adjust layout and show all plots at once
plt.tight_layout()
plt.show()

