#include <iostream>
#include <vector>
#include <unordered_map>
#include <climits>
#include <utility>

using namespace std;

const int INFINITY_COST = 1e2;
bool f = 0;
bool splitHorizonbool = 1;
class Router;

// Network class for holding routers and initializing network topology
class Network
{
public:
	vector<Router> routers;

	Network(int numRouters);

	void addEdge(int src, int dest, int cost);
	void printNetworkRoutingTables() const;
};

// Router class representing each node in the network
class Router
{
public:
	int id;
	unordered_map<int, int> distanceTable;								// Distance to each destination
	unordered_map<int, int> nextHopTable;								// Next hop for each destination
	unordered_map<int, unordered_map<int, int>> neighborDistanceTables; // State of neighbors' distances to each destination
	vector<pair<int, int>> neighbors;									// (Neighbor id, cost to neighbor)
	int numRouters;
	Network *network;

	Router(int id, Network *network, int numRouters) : id(id), network(network), numRouters(numRouters)
	{
		// Initialize the distance table with infinity for all routers except itself
		for (int i = 0; i < numRouters; ++i)
		{
			distanceTable[i] = (i == id) ? 0 : INFINITY_COST;
			// neighborDistanceTables[i][]
			nextHopTable[i] = (i == id) ? id : -1; // Set -1 for unknown next hops initially
			for (int j = 0; j < numRouters; j++)
			{
				neighborDistanceTables[i][j] = i == j ? 0 : INFINITY_COST;
			}
		}
	}

	void addNeighbor(int neighbor, int cost)
	{
		neighbors.emplace_back(neighbor, cost);
		distanceTable[neighbor] = cost;
		nextHopTable[neighbor] = neighbor;
		// Initialize neighbor's distance table in state tracking
		neighborDistanceTables[neighbor] = unordered_map<int, int>();
		broadcastUpdates(neighbor);
		// neighborDistanceTables[neighbor][id] = cost ;
	}

	bool updateDistanceTable(int dest, int newCost, int nextHop)
	{

		//          if(id == 1 && dest ==2 ){
		//     cerr<<distanceTable[dest]<<" "<<newCost<<" ";
		//     cerr<<nextHopTable[dest]<<" "<< nextHop<<endl;
		//  }

		// if (newCost < distanceTable[dest]) {
		//     distanceTable[dest] = newCost;
		//     nextHopTable[dest] = nextHop;
		//     return true;  // Table updated
		// }
		// return false;  // No update

		// if(id == 0 && dest ==4 ) cerr<<distanceTable[dest]<<" "<<newCost<<endl;
		if (f == 1 && id == 3 && dest == 4)
		{
			cerr << distanceTable[dest] << endl;
		}

		if (newCost != distanceTable[dest] || nextHopTable[dest] != nextHop)
		{
			if (id == dest)
			{
				distanceTable[dest] = 0;
				nextHopTable[dest] = id;
				return 1;
			}

			distanceTable[dest] = newCost;
			nextHopTable[dest] = nextHop;
			return true;
		}
		return false;
	}

	void reCalculate()
	{
		for (int dest = 0; dest < numRouters; dest++)
		{
			int bestCost = INFINITY_COST;
			int bestNextHop = -1;
			for (const auto &[neighbor, costToNeighbor] : neighbors)
			{
				int neighborDistance = neighborDistanceTables[neighbor].count(dest) ? neighborDistanceTables[neighbor][dest] : INFINITY_COST;
				int calculatedCost = costToNeighbor + neighborDistance;
				if (calculatedCost < bestCost)
				{
					if (splitHorizonbool)
					{
						if (network->routers[neighbor].nextHopTable[dest] == id)
						{
							continue;
						}
					}
					bestCost = calculatedCost;
					bestNextHop = neighbor;
				}
			}
			if (dest == id)
			{
				bestCost = 0;
				bestNextHop = id;
			}

			if (distanceTable[dest] != bestCost || nextHopTable[dest] != bestNextHop)
			{
				distanceTable[dest] = bestCost;
				nextHopTable[dest] = bestNextHop;
				broadcastUpdates(dest);
			}
		}
	}

	void receiveUpdate(int neighborId, int dest, int newCost)
	{
		// Update the stored state information of the neighbor
		neighborDistanceTables[neighborId][dest] = newCost;

		// Recalculate the distance to the destination using the updated information
		int bestCost = INFINITY_COST;
		int bestNextHop = -1;

		for (const auto &[neighbor, costToNeighbor] : neighbors)
		{
			int neighborDistance = neighborDistanceTables[neighbor].count(dest) ? neighborDistanceTables[neighbor][dest] : INFINITY_COST;
			int calculatedCost = costToNeighbor + neighborDistance;
			if (calculatedCost < bestCost)
			{
				bestCost = calculatedCost;
				bestNextHop = neighbor;
			}
		}

		// distanceTable[dest] =  bestCost;
		// nextHopTable[dest] = bestNextHop ;
		//  broadcastUpdates(dest);

		// Update distance table if a shorter path is found
		if (updateDistanceTable(dest, bestCost, bestNextHop))
		{

			broadcastUpdates(dest);
		}
	}
	void reversePoisoning(int dest, int neighbor)
	{
		int broadcastedCost = (nextHopTable[dest] == neighbor) ? INFINITY_COST : distanceTable[dest];
		network->routers[neighbor].receiveUpdate(id, dest, broadcastedCost);
	}
	// void splitHorizon(int dest  , int neighbor ){
	//     if(nextHopTable[dest] == neighbor) return ;
	//     network->routers[neighbor].receiveUpdate(id, dest,  distanceTable[dest] );

	// }

	// Each router broadcasts its own updates to neighbors
	void broadcastUpdates(int dest)
	{
		for (const auto &[neighbor, cost] : neighbors)
		{
			// normal
			network->routers[neighbor].receiveUpdate(id, dest, distanceTable[dest]);
			// reversePoisoning
			//  reversePoisoning(dest , neighbor) ;
			// splitHorizon
			//  splitHorizon(dest , neighbor) ;
		}
	}

	void printRoutingTable() const
	{
		cout << "Routing table for Router " << id + 1 << ":\n";
		for (const auto &[dest, cost] : distanceTable)
		{
			cout << "  Destination: " << dest + 1 << " | Cost: " << cost
				 << " | Next Hop: " << (nextHopTable.at(dest) == -1 ? "Unknown" : to_string(nextHopTable.at(dest) + 1)) << endl;
		}
	}
};

Network::Network(int numRouters)
{
	for (int i = 0; i < numRouters; ++i)
	{
		routers.emplace_back(i, this, numRouters);
	}
}

void Network::addEdge(int src, int dest, int cost)
{
	routers[src].addNeighbor(dest, cost);
	routers[dest].addNeighbor(src, cost);
}

void Network::printNetworkRoutingTables() const
{
	for (const auto &router : routers)
	{
		router.printRoutingTable();
		cout << endl;
	}
}

int main()
{
	freopen("input.txt", "r", stdin);
	freopen("output.txt", "w", stdout);
	freopen("error.txt", "w", stderr);
	// int numRouters = 5; // Number of routers (nodes)
	// Network network(numRouters);
	// network.addEdge(0, 1, 3);
	// network.addEdge(0, 2, 5);
	// network.addEdge(1, 2, 2);
	// network.addEdge(1, 3, 4);
	// network.addEdge(2, 3, 1);
	// network.addEdge(3, 4, 6);
	// Initialize edges (src, dest, cost)
	int n, m;
	cin >> n >> m;
	int numRouters = n; // Number of routers (nodes)
	Network network(numRouters);

	// Initialize edges (src, dest, cost)
	while (m--)
	{
		int u, v, c;
		cin >> u >> v >> c;
		network.addEdge(u - 1, v - 1, c);
	}

	// Initial broadcast
	for (auto &router : network.routers)
	{
		for (const auto &[dest, cost] : router.distanceTable)
		{
			router.broadcastUpdates(dest);
		}
	}

	cout << "Initial Routing Tables:" << endl;
	network.printNetworkRoutingTables();

	// Simulate link failure between nodes 3 and 4
	cout << "\nSimulating Link Failure between Nodes 4 and 5" << endl;
	network.routers[3].distanceTable[4] = INFINITY_COST;
	network.routers[4].distanceTable[3] = INFINITY_COST;
	for (auto &[dest, cost] : network.routers[3].neighbors)
	{
		if (dest == 4)
			cost = INFINITY_COST;
	}
	for (auto &[dest, cost] : network.routers[4].neighbors)
	{
		if (dest == 3)
			cost = INFINITY_COST;
	}
	f = 1;
	network.routers[3].reCalculate();
	network.routers[4].reCalculate();
	// Print final routing tables after propagation
	cout << "\nRouting Tables after Link Failure Propagation:" << endl;
	network.printNetworkRoutingTables();

	return 0;
}