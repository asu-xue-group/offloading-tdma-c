#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "global.h"
#include "knapsack.hpp"
#include "output.hpp"
#include "structs.h"
#include "subs.hpp"
#include "graph.hpp"


//#define debug
//#define print_solution

using json = nlohmann::json;
namespace fs = std::filesystem;
SERVER *s;
USER *u;
OPT *opt;
std::vector<std::vector<OPT_PATH *>> opt_path;
int K, M, L, N;
long long table_size;
Graph graph;


int main(int argc, char **argv) {
    std::chrono::steady_clock::time_point begin_initial = std::chrono::steady_clock::now();
    int flag;
    graph = Graph();
    opt_path.push_back(std::vector<OPT_PATH *>());

    // Check commandline arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ifile> <flag>\n", argv[0]);
        exit(1);
    }
    flag = std::stoi(argv[2]);
    std::ifstream is(argv[1]);
    json j = json::parse(is);

    std::string mode_str = "original";
    if (flag == 1) {
        mode_str = "relaxed";
    } else if (flag == -1) {
        mode_str = "restricted";
    }

    // Initialize the csv file
    fs::path p = argv[1];
    auto csv_file = p.parent_path().parent_path().parent_path() / "results.csv";
    if (!fs::exists(csv_file)) {
        FILE *fp = fopen(csv_file.string().c_str(), "a");
        if (!fp) {
            fprintf(stderr, "Error: cannot open file %s\n", csv_file.string().c_str());
            exit(0);
        }
        fprintf(fp, "%s,%s,%s,%s,%s,%s\n", "mode", "tc", "lambda", "reward", "time", "t_size");
        fclose(fp);
    }


    std::string name = j["name"];
    K = j["num_user_tier"];
    M = j["num_server"];
    L = j["num_relay"];
    N = j["num_user"];

    std::cout << std::format("Running test case {} with flag {}, T={}, z={}\n",
                             argv[1], mode_str, T, z) << std::endl;

    // Initialize the data structures
    // Here relays are considered as servers
    s = new SERVER[M + L + 1];
    u = new USER[N + 1];

    for (int m = 1; m <= M + L; m++) {
        int cpu, ram;

        if (m <= M) {
            s[m].index = m;
            s[m].x = j["servers"][m - 1]["x"];
            s[m].y = j["servers"][m - 1]["y"];
            cpu = j["servers"][m - 1]["cpu"];
            ram = j["servers"][m - 1]["ram"];
        } else {
            s[m].index = m;
            s[m].x = j["relays"][m - M - 1]["x"];
            s[m].y = j["relays"][m - M - 1]["y"];
            cpu = j["relays"][m - M - 1]["cpu"];
            ram = j["relays"][m - M - 1]["ram"];
        }

        s[m].cpu_orig = cpu;
        s[m].ram_orig = ram;
        // Only do the scaling if the flag is not 0 and the original values are greater than lambda
        if (flag == 0 || cpu <= lambda) {
            s[m].cpu = cpu;
            s[m].cpu_scaled = false;
        } else {
            s[m].cpu = lambda;
            s[m].cpu_scaled = true;
        }
        if (flag == 0 || ram <= lambda) {
            s[m].ram = ram;
            s[m].ram_scaled = false;
        } else {
            s[m].ram = lambda;
            s[m].ram_scaled = true;
        }

        if (m > M) {
            s[m].index = m - M;
            s[m].relay = true;
            s[m].name = std::format("r_{}", m - M);
        } else {
            s[m].index = m;
            s[m].relay = false;
            s[m].name = std::format("s_{}", m);
        }

        // Create a graph node for the server/relay
        graph.add_node(&s[m]);
    }

    for (int n = 1; n <= N; n++) {
        u[n].index = n;
        u[n].x = j["users"][n - 1]["x"];
        u[n].y = j["users"][n - 1]["y"];
        u[n].ddl = j["users"][n - 1]["ddl"];
        u[n].data = j["users"][n - 1]["data"];
        u[n].cpu = j["users"][n - 1]["local_cpu"];
        u[n].ram = j["users"][n - 1]["local_ram"];

        u[n].tier[1].tier = 1;
        u[n].tier[1].cpu = j["users"][n - 1]["tiers"][0]["cpu"];
        u[n].tier[1].ram = j["users"][n - 1]["tiers"][0]["ram"];
        u[n].tier[1].reward = j["users"][n - 1]["tiers"][0]["reward"];
        u[n].tier[1].time = j["users"][n - 1]["tiers"][0]["time"];

        u[n].tier[2].tier = 2;
        u[n].tier[2].cpu = j["users"][n - 1]["tiers"][1]["cpu"];
        u[n].tier[2].ram = j["users"][n - 1]["tiers"][1]["ram"];
        u[n].tier[2].reward = j["users"][n - 1]["tiers"][1]["reward"];
        u[n].tier[2].time = j["users"][n - 1]["tiers"][1]["time"];

        u[n].name = std::format("u_{}", n);
        graph.add_node(&u[n]);

        opt_path.push_back(std::vector<OPT_PATH *>());
        opt_path[n].push_back(nullptr);
        for (int k = 1; k <= K; k++) {
            opt_path[n].push_back(new OPT_PATH());
        }
    }

    // Attempt to add edges to every single connection from user to server/relay and relay to relay.
    for (int n = 1; n <= N; n++) {
        for (int m = 1; m <= M + L; m++) {
            graph.add_edge(&u[n], &s[m]);
        }
    }
    for (int l = M + 1; l <= M + L; l++) {
        for (int m = 1; m <= M + L; m++) {
            if (l == m) continue;
            graph.add_edge(&s[l], &s[m]);
        }
    }

    table_size = 1;
    for (int m = 1; m <= M + L; m++) {
        table_size = table_size * (1 + s[m].cpu) * (1 + s[m].ram);
    }
    table_size = table_size * (1 + T) * (1 + N);
    std::cout << std::format("Table size: {}\n", table_size) << std::endl;
    opt = new OPT[table_size];

    std::chrono::steady_clock::time_point end_prep = std::chrono::steady_clock::now();
    std::cout << std::format("Preprocessing took {} seconds\n",
                             std::chrono::duration_cast<std::chrono::microseconds>(end_prep - begin_initial).count() / 1000000.0) << std::endl;

    // The raw results are stored in the `opt` variable
    dp(flag);

    auto solution = trace_solution(opt, flag, N);
    auto reward = std::get<float>(solution.back().back());
    auto out_path = p.parent_path() / std::format("output_{}_T{}_L{}.txt", mode_str, T, lambda);

    auto end_overall = std::chrono::steady_clock::now();
    auto time_delta = std::chrono::duration_cast<std::chrono::microseconds>(end_overall - begin_initial).count() / 1000000.0;

    print_to_file(out_path.string(), solution, time_delta);
    result_to_csv(csv_file, mode_str, p.parent_path().stem().string(), lambda, reward,time_delta, table_size);

    std::cout << "==================================\n";

#ifdef print_solution
    print_results(solution, N);
#endif

//    std::cout << "Time taken: " << time_delta << " seconds\n";
//    std::cout << "Table size: " << table_size << std::endl;

    free(s);
    free(u);
    free(opt);

    return 0;
}
