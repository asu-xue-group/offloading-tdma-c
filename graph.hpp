#ifndef TDMA_KNAPSACK_GRAPH_HPP
#define TDMA_KNAPSACK_GRAPH_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <limits>
#include <queue>
#include <algorithm>

using Entity = std::variant<SERVER*, USER*>;

struct Node {
    std::string name;
    Entity repr;

    [[nodiscard]] bool is_server() const {
        return std::holds_alternative<SERVER*>(repr);
    }

    [[nodiscard]] SERVER* as_server() const {
        return std::get<SERVER*>(repr);
    }

    [[nodiscard]] USER* as_user() const {
        return std::get<USER*>(repr);
    }
};

struct Edge {
    Node &to;
    int timeslot;
    double snr;
    double distance;
};

class Graph {
    std::vector<std::vector<Edge>> adj;         // adj[u] = list of (v, w)
    std::unordered_map<std::string,int> index;  // map names → 0..N-1
    std::vector<std::string> names;             // inverse: id → name

public:
    // add a node if new, return its ID
    int addNode(const Entity ent) {
        if (auto it = index.find(ent); it != index.end())
            return it->second;
        int id = static_cast<int>(names.size());
        names.push_back(str);
        index[str] = id;
        adj.emplace_back();       // add an empty neighbor list
        return id;
    }

    void addEdge(const std::string& a, const std::string& b) {
        int e1 = addNode(a);
        int e2 = addNode(b);
        adj[e1].push_back({e2});
    }

    // Dijkstra works on ints [0..n). Return nullopt if A or B is not in the graph.
    std::optional<std::pair<std::vector<std::string>,double>>
    shortest_path(const std::string& A,const std::string& B) const {
        auto iA = index.find(A), iB = index.find(B);
        if (iA==index.end() || iB==index.end())
            return std::nullopt;           // src/dst not even in the graph

        int n = static_cast<int>(adj.size()), s = iA->second, t = iB->second;
        const double INF = std::numeric_limits<double>::infinity();
        std::vector<double>  dist(n, INF);
        std::vector<int>     prev(n, -1);

        using PDI = std::pair<double,int>;
        std::priority_queue<PDI, std::vector<PDI>, std::greater<>> pq;

        dist[s]=0;
        pq.emplace(0,s);

        while(!pq.empty()) {
            auto [d,u] = pq.top(); pq.pop();
            if (d>dist[u]) continue;
            if (u==t) break;
            for (auto& e: adj[u]) {
                double alt = d + e.weight;
                if (alt < dist[e.to]) {
                    dist[e.to] = alt;
                    prev[e.to] = u;
                    pq.emplace(alt, e.to);
                }
            }
        }

        if (dist[t]==INF)
            return std::make_pair(std::vector<std::string>{}, INF);

        // rebuild path of names
        std::vector<std::string> path;
        for (int cur=t; cur!=-1; cur=prev[cur])
            path.push_back(names[cur]);
        std::ranges::reverse(path);
        return std::make_pair(std::move(path), dist[t]);
    }
};

#endif //TDMA_KNAPSACK_GRAPH_HPP
