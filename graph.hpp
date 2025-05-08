#ifndef TDMA_KNAPSACK_GRAPH_HPP
#define TDMA_KNAPSACK_GRAPH_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <limits>
#include <queue>
#include <algorithm>
#include <variant>
#include "structs.h"
#include "global.h"
#include "subs.hpp"

#define Entity std::variant<SERVER*, USER*>

class Graph {
private:
    struct Node {
        std::string name;
        Entity entity;

        [[nodiscard]] bool is_server() const { return std::holds_alternative<SERVER*>(entity); }
        [[nodiscard]] SERVER* as_server() const { return std::get<SERVER*>(entity); }
        [[nodiscard]] USER* as_user() const { return std::get<USER*>(entity); }
    };

    struct Edge {
        int to_id;         // Target node ID
        int timeslot;      // Time slot (distance metric)
        double snr;        // Signal-to-noise ratio
        double distance;   // Physical distance
    };

    static bool ent_equals(const Entity &e1, const Entity &e2) {
        return std::visit([](auto* ptr) { return ptr->name; }, e1) ==
                std::visit([](auto* ptr) { return ptr->name; }, e2);
    }


    std::vector<Node> nodes;                        // List of all nodes
    std::vector<std::vector<Edge>> adj_list;        // Adjacency list
    std::unordered_map<std::string, int> node_map;  // Name to node ID mapping

public:
    // Add a node to the graph (if not already present)
    int add_node(const Entity& entity) {
        std::string name = std::visit([](auto* ptr) { return ptr->name; }, entity);

        // Check if the node already exists
        auto it = node_map.find(name);
        if (it != node_map.end()) {
            return it->second;
        }

        // Add a new node
        int id = static_cast<int>(nodes.size());
        nodes.push_back({name, entity});
        node_map[name] = id;
        adj_list.emplace_back();  // Add an empty adjacency list

        return id;
    }

    // Add an edge between two nodes with time slot as distance metric
    bool add_edge(const Entity& from,
                  const Entity& to) {
        // Reject self-loops
        if (ent_equals(from, to)) return false;

        // Get positions
        auto get_pos = [](const auto& entity) {
            return std::visit([](auto* ptr) {
                return std::pair{ptr->x, ptr->y};
            }, entity);
        };

        auto [from_x, from_y] = get_pos(from);
        auto [to_x, to_y] = get_pos(to);

        // Calculate distance and SNR
        double distance = calc_distance(from_x, from_y, to_x, to_y);
        double snr = calc_snr(distance);

        // Skip edges with insufficient SNR
        if (snr < beta) {
            return false;
        }

        // Add nodes and edge
        int from_id = add_node(from);
        int to_id = add_node(to);

        // Check if edge already exists
        for (const auto& edge : adj_list[from_id]) {
            if (edge.to_id == to_id) {
                // Edge already exists, don't add a duplicate
                return false;
            }
        }

        adj_list[from_id].push_back({to_id, INT_MAX, snr, distance});
        return true;
    }

    // Find the shortest path between two nodes using timeslot as a distance metric
    std::optional<std::tuple<std::vector<std::string>, std::vector<int>, int>>
    shortest_path(const std::string& start, const std::string& end) {
        // Check if nodes exist
        auto start_it = node_map.find(start);
        auto end_it = node_map.find(end);
        if (start_it == node_map.end() || end_it == node_map.end()) {
            return std::nullopt;
        }

        int start_id = start_it->second;
        int end_id = end_it->second;
        int n = static_cast<int>(nodes.size());

        // Initialize Dijkstra data structures
        const int INF = std::numeric_limits<int>::max();
        std::vector<int> dist(n, INF);
        std::vector<int> prev(n, -1);

        // Priority queue: <distance, node_id>
        using PQueue = std::priority_queue<
                std::pair<int, int>,
                std::vector<std::pair<int, int>>,
                std::greater<>
        >;
        PQueue pq;

        // Start Dijkstra
        dist[start_id] = 0;
        pq.emplace(0, start_id);

        while (!pq.empty()) {
            auto [d, uu] = pq.top();
            pq.pop();

            // Skip outdated entries
            if (d > dist[uu]) continue;

            // Early termination if we reached destination
            if (uu == end_id) break;

            // Explore neighbors
            for (const auto& edge : adj_list[uu]) {
                int v = edge.to_id;
                int new_dist = d + edge.timeslot;

                if (new_dist < dist[v]) {
                    dist[v] = new_dist;
                    prev[v] = uu;
                    pq.emplace(new_dist, v);
                }
            }
        }

        // Check if a path exists
        if (dist[end_id] == INF) {
            return std::make_tuple(std::vector<std::string>{}, std::vector<int>{}, INF);
        }

        // Reconstruct the path and collect timeslots
        std::vector<std::string> path;
        std::vector<int> timeslots;
        for (int cur = end_id; cur != -1; cur = prev[cur]) {
            path.push_back(nodes[cur].name);
            if (prev[cur] != -1) {
                for (const auto &edge: adj_list[prev[cur]]) {
                    if (edge.to_id == cur) {
                        timeslots.push_back(edge.timeslot);
                        break;
                    }
                }
            }
        }
        std::reverse(path.begin(), path.end());
        std::reverse(timeslots.begin(), timeslots.end());

        return std::make_tuple(path, timeslots, dist[end_id]);
    }

    // Here we assume that the transmission over multihop occurs simultaneously
    int update_timeslot(const float data, const float time) {
        // X_ub is the maximum # of timeslots this transmission can use given the available time
        auto X_ub = std::floor(time / (T * z));

        // Each edge has a weight (timeslot) corresponding to the X_ub
        for (auto &source: adj_list) {
            for (auto &dest : source) {
                dest.timeslot = std::ceil(data / (X_ub * z * bandwidth * log2(1 + dest.snr)));
            }
        }

        return static_cast<int>(X_ub);
    }

    // Get node details by name
    std::optional<Node> get_node(const std::string& name) const {
        auto it = node_map.find(name);
        if (it == node_map.end()) {
            return std::nullopt;
        }
        return nodes[it->second];
    }

    // Get all connections for a node
    std::vector<std::pair<std::string, int>> get_connections(const std::string& name) const {
        auto it = node_map.find(name);
        if (it == node_map.end()) {
            return {};
        }

        std::vector<std::pair<std::string, int>> connections;
        for (const auto& edge : adj_list[it->second]) {
            connections.emplace_back(nodes[edge.to_id].name, edge.timeslot);
        }
        return connections;
    }
    
    // See if two nodes are directly connected
    std::optional<Edge> is_connected(const std::string &from, const std::string &to) const {
        auto from_it = node_map.find(from);
        auto to_it = node_map.find(to);

        // If either node doesn't exist, they're not connected
        if (from_it == node_map.end() || to_it == node_map.end()) {
            return std::nullopt;
        }

        // Check if there's a direct edge from 'from' to 'to'
        for (const auto &edge: adj_list[from_it->second]) {
            if (edge.to_id == to_it->second) {
                return edge;
            }
        }

        return std::nullopt;
    }

};

#endif //TDMA_KNAPSACK_GRAPH_HPP