#include <bits/stdc++.h>
#include <iostream>
#include <unordered_map>
#include <list>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

// Cache node structure
struct CacheNode
{
    string url;
    string content;
    CacheNode(string u, string c) : url(u), content(c) {}
};

class LRUCache
{
    int capacity;
    list<CacheNode> cacheList;
    unordered_map<string, list<CacheNode>::iterator> cacheMap;

public:
    LRUCache(int cap) : capacity(cap) {}

    string get(const string &url)
    {
        if (cacheMap.find(url) == cacheMap.end())
        {
            return ""; // Cache miss
        }
        else
        {
            // Cache hit, move the accessed node to the front
            cacheList.splice(cacheList.begin(), cacheList, cacheMap[url]);
            return cacheMap[url]->content;
        }
    }

    void put(const string &url, const string &content)
    {
        if (cacheMap.find(url) == cacheMap.end())
        {
            if (cacheList.size() == capacity)
            {
                // Remove least recently used element
                cacheMap.erase(cacheList.back().url);
                cacheList.pop_back();
            }
            // Insert new node at the front
            cacheList.push_front(CacheNode(url, content));
            cacheMap[url] = cacheList.begin();
        }
        else
        {
            // Update existing node and move to front
            cacheMap[url]->content = content;
            cacheList.splice(cacheList.begin(), cacheList, cacheMap[url]);
        }
    }

    void displayCache() const
    {
        cout << "Cache contents [Most to Least Recently Used]:\n";
        for (const auto &node : cacheList)
        {
            cout << node.url << "\n";
        }
    }
};

enum DNSServer
{
    ROOT,
    TLD,
    AUTHORITATIVE
};

// Simulating DNS Resolver and Cache
map<string, string> cacheMap;

string checkLocalCache(const string &domain)
{
    if (cacheMap.find(domain) != cacheMap.end())
    {
        cout << "IP address found in local cache.\n";
        return cacheMap[domain];
    }
    cout << "IP address not found in local cache. Sending query to DNS resolver.\n";
    return "";
}

string dnsLookup(DNSServer server, const string &query)
{
    int delay = rand() % 500 + 100;

    // Define a timeout threshold (e.g., 400 milliseconds)
    int timeoutThreshold = 400;

    // If the delay exceeds the timeout, simulate query failure due to timeout
    if (delay > timeoutThreshold)
    {
        cout << "DNS query timed out after " << delay << " ms at "
             << (server == ROOT ? "ROOT" : server == TLD ? "TLD"
                                                         : "AUTHORITATIVE")
             << " server.\n";
        return ""; // Return empty string to indicate failure due to timeout
    }

    // Simulate the actual delay for the query
    this_thread::sleep_for(chrono::milliseconds(delay));

    if (rand() % 10 < 1) // Randomly simulate DNS query failure
    {
        return ""; // Simulate a failed DNS query
    }

    // Handle the query based on the server type
    if (server == ROOT)
    {
        // Extract the TLD from the domain (e.g., "com" from "example.com")
        string tld = query.substr(query.find_last_of('.') + 1);
        cout << "Querying Root DNS Server for TLD: " << tld << "\n";

        // Check if TLD is recognized
        if (tld == "com" || tld == "uk" || tld == "org" || tld == "in")
        {
            // Recursively call the TLD server for further resolution
            return dnsLookup(TLD, query);
        }
        else
        {
            cout << "TLD not recognized by Root server.\n";
            return ""; // TLD not found
        }
    }
    else if (server == TLD)
    {
        // Querying the TLD server with the full domain (e.g., "example.com")
        cout << "Querying TLD DNS Server for domain: " << query << "\n";

        // Recognize certain domains and delegate to the Authoritative server
        if (query == "www.example.com" || query == "www.google.com" || query == "www.facebook.com" || query == "www.youtube.com" || query == "www.wikipedia.org" || query == "www.iitg.ac.in" || query == "www.amazon.com" || query == "www.neverssl.com" || query == "www.codeforces.com")
        {
            // Recursively call the Authoritative server for final resolution
            return dnsLookup(AUTHORITATIVE, query);
        }
        else
        {
            cout << "Domain not recognized by TLD server.\n";
            return ""; // Domain not found
        }
    }
    else if (server == AUTHORITATIVE)
    {
        // Authoritative server provides the final IP address for recognized domains
        cout << "Querying Authoritative DNS Server for final resolution of: " << query << "\n";

        if (query == "www.example.com")
            return "93.184.215.14"; // Example website
        if (query == "www.google.com")
            return "142.250.195.36"; // Google
        if (query == "www.facebook.com")
            return "163.70.143.35"; // Facebook
        if (query == "www.youtube.com")
            return "142.250.72.238"; // YouTube
        if (query == "www.wikipedia.org")
            return "103.102.166.224"; // Wikipedia
        if (query == "www.iitg.ac.in")
            return "172.17.0.22"; // IITG
        if (query == "www.amazon.com")
            return "205.251.215.216"; // Amazon
        if (query == "www.neverssl.com")
            return "34.223.124.45"; // NeverSSL
        if (query == "www.codeforces.com")
            return "172.67.68.254"; // CodeForces
    }

    // Default empty response if none of the cases match
    return "";
}

string backupDnsLookup(DNSServer server, const string &query)
{
    this_thread::sleep_for(chrono::milliseconds(rand() % 500 + 100)); // Simulate network delay

    // Handle the query based on the server type
    if (server == ROOT)
    {
        // Extract the TLD from the domain (e.g., "com" from "example.com")
        string tld = query.substr(query.find_last_of('.') + 1);
        cout << "Querying Root DNS Server for TLD: " << tld << "\n";

        // Check if TLD is recognized
        if (tld == "com" || tld == "uk" || tld == "org" || tld == "in")
        {
            // Recursively call the TLD server for further resolution
            return backupDnsLookup(TLD, query);
        }
        else
        {
            cout << "TLD not recognized by Root server.\n";
            return ""; // TLD not found
        }
    }
    else if (server == TLD)
    {
        // Querying the TLD server with the full domain (e.g., "example.com")
        cout << "Querying TLD DNS Server for domain: " << query << "\n";

        // Recognize certain domains and delegate to the Authoritative server
        if (query == "www.example.com" || query == "www.google.com" || query == "www.facebook.com" || query == "www.youtube.com" || query == "www.wikipedia.org" || query == "www.iitg.ac.in" || query == "www.amazon.com" || query == "www.neverssl.com" || query == "www.codeforces.com")
        {
            // Recursively call the Authoritative server for final resolution
            return backupDnsLookup(AUTHORITATIVE, query);
        }
        else
        {
            cout << "Domain not recognized by TLD server.\n";
            return ""; // Domain not found
        }
    }
    else if (server == AUTHORITATIVE)
    {
        // Authoritative server provides the final IP address for recognized domains
        cout << "Querying Authoritative DNS Server for final resolution of: " << query << "\n";

        if (query == "www.example.com")
            return "93.184.216.34"; // Example website
        if (query == "www.google.com")
            return "74.125.236.195"; // Google
        if (query == "www.facebook.com")
            return "157.240.229.35"; // Facebook
        if (query == "www.youtube.com")
            return "142.250.72.238"; // YouTube
        if (query == "www.wikipedia.org")
            return "103.102.166.224"; // Wikipedia
        if (query == "www.iitg.ac.in")
            return "172.17.0.22"; // IITG
        if (query == "www.amazon.com")
            return "205.251.215.216"; // Amazon
        if (query == "www.neverssl.com")
            return "34.223.124.45"; // NeverSSL
        if (query == "www.codeforces.com")
            return "172.67.68.254"; // CodeForces
    }

    // Default empty response if none of the cases match
    return "";
}

string resolveDNS(const string &domain)
{
    // Check if the IP address is available in the local cache
    string ipAddress = checkLocalCache(domain);

    if (ipAddress.empty()) // If not found in cache, proceed with DNS resolution
    {
        cout << "Starting DNS resolution for " << domain << "...\n";

        // Initiate recursive DNS lookup starting from the ROOT server
        ipAddress = dnsLookup(ROOT, domain);

        if (ipAddress.empty()) // If DNS lookup fails, print an error
        {
            cout << "DNS resolution failed for " << domain << ".\n";
            return "";
        }

        // If the lookup is successful, cache the resolved IP address
        cacheMap[domain] = ipAddress;
    }
    else
    {
        cout << "IP address found in local cache for " << domain << ": " << ipAddress << "\n";
    }

    // Return the resolved IP address
    return ipAddress;
}

string backupResolveDNS(const string &domain)
{
    // Check if the IP address is available in the local cache
    string ipAddress = checkLocalCache(domain);

    if (ipAddress.empty()) // If not found in cache, proceed with DNS resolution
    {
        cout << "Starting DNS resolution for " << domain << "...\n";

        // Initiate recursive DNS lookup starting from the ROOT server
        ipAddress = backupDnsLookup(ROOT, domain);

        if (ipAddress.empty()) // If DNS lookup fails, print an error
        {
            cout << "DNS resolution failed for " << domain << ".\n";
            return "";
        }

        // If the lookup is successful, cache the resolved IP address
        cacheMap[domain] = ipAddress;
    }
    else
    {
        cout << "IP address found in local cache for " << domain << ": " << ipAddress << "\n";
    }

    // Return the resolved IP address
    return ipAddress;
}

string httpGet(const string &ip, const string &url)
{
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[4096];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Socket creation failed\n";
        return "";
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);

    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        cerr << "Invalid address/Address not supported\n";
        close(sockfd);
        return "";
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Connection Failed\n";
        close(sockfd);
        return "";
    }

    string request = "GET / HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";
    send(sockfd, request.c_str(), request.length(), 0);

    string response;
    int bytes_received;
    while ((bytes_received = read(sockfd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_received] = '\0';
        response += buffer;
    }

    close(sockfd);
    return response;
}

int main()
{
    srand(time(0));

    vector<string> domains = {
        "www.neverssl.com",
        "www.neverssl.com",
        "www.codeforces.com",
        "www.wikipedia.org",
        "www.iitg.ac.in"};

    LRUCache cache(5); // Cache with capacity of 5 pages

    for (const auto &domain : domains)
    {
        string content = cache.get(domain);
        if (content.empty())
        {
            cout << "Cache miss. Resolving DNS for " << domain << "...\n";
            string ipAddress = resolveDNS(domain);

            if (!ipAddress.empty())
            {
                cout << "Fetching page from server " << domain << "...\n";
                content = httpGet(ipAddress, domain);
                cache.put(domain, content);
            }
            else
            {
                cerr << "Primary server DNS resolution failed. Switching to the backup server to ensure seamless connectivity.\n"
                     << endl;
                string ipAddress = backupResolveDNS(domain);

                if (!ipAddress.empty())
                {
                    cout << "Fetching page from server " << domain << "...\n";
                    content = httpGet(ipAddress, domain);
                    cache.put(domain, content);
                }
            }
        }
        else
        {
            cout << "Cache hit. Returning cached content for " << domain << ".\n";
        }

        cout << "Page Content for " << domain << ":\n"
             << content.substr(0, 10000) << "...\n"; // Limiting output for brevity
        for (int i = 0; i < 10; i++)
        {
            cout << endl;
        }
    }

    cache.displayCache();

    return 0;
}
