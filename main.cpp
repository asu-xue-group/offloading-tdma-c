#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <regex>
#include <nlohmann/json.hpp>
#include "global.h"
#include "knapsack.hpp"
#include "output.hpp"
#include "structs.h"
#include "subs.hpp"


//#define debug
//#define print_solution

using json = nlohmann::json;
namespace fs = std::filesystem;
SERVER *s;
USER *u;
OPT *opt;
std::vector<std::vector<double>> s_distance, ur_distance, rs_distance;
std::vector<std::vector<TIMING *>> timing;
int K, M, L, N;
long long table_size;

int main(int argc, char **argv) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    int flag;
    double min_time = INT_MAX, max_time = 0; // minimum and maximum transmission time

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
    auto csv_file = p.parent_path().parent_path() / "results.csv";
    FILE *fp = fopen(csv_file.string().c_str(), "a");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file %s\n", csv_file.string().c_str());
        exit(0);
    }

    fprintf(fp, "%s,%s,%s,%s,%s,%s\n", "mode", "tc_num", "X", "lambda", "reward", "time");


    std::string name = j["name"];
    K = j["num_user_tier"];
    M = j["num_server"];
    L = j["num_relay"];
    N = j["num_user"];

    std::cout << std::format("Running test case {} with flag {}, T={}, X={}, z={}\n",
                             argv[1], flag, T, X, z) << std::endl;

    // Initialize the data structures
    // Here relays are considered as servers
    s = new SERVER[M + L + 1];
    u = new USER[N + 1];
    s_distance = std::vector<std::vector<double>>(M + 1);
    ur_distance = std::vector<std::vector<double>>(L + 1);
    rs_distance = std::vector<std::vector<double>>(L + 1);
    timing = std::vector<std::vector<TIMING *>>(N + 1);
    for (int i = 0; i <= N; i++) {
        timing.at(i) = std::vector<TIMING *>(M + 1);
    }

    for (int m = 1; m <= M + L; m++) {
        if (m <= M) {
            s[m].index = m;
            s[m].x = j["servers"][m - 1]["x"];
            s[m].y = j["servers"][m - 1]["y"];
            s[m].cpu = j["servers"][m - 1]["cpu"];
            s[m].ram = j["servers"][m - 1]["ram"];
            s_distance.at(m) = std::vector<double>(N + 1);
        } else {
            s[m].index = m;
            s[m].x = j["relays"][m - M - 1]["x"];
            s[m].y = j["relays"][m - M - 1]["y"];
            s[m].cpu = j["relays"][m - M - 1]["cpu"];
            s[m].ram = j["relays"][m - M - 1]["ram"];
            ur_distance.at(m - M) = std::vector<double>(N + 1);
            rs_distance.at(m - M) = std::vector<double>(M + 1);
        }
    }

    for (int n = 1; n <= N; n++) {
        u[n].index = n;
        u[n].x = j["users"][n - 1]["x"];
        u[n].y = j["users"][n - 1]["y"];
        u[n].ddl = j["users"][n - 1]["ddl"];
        u[n].data = j["users"][n - 1]["data"];

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

        // Calculate the minimum and maximum transmission time
        if (u[n].ddl - u[n].tier[2].time > max_time) {
            max_time = u[n].ddl - u[n].tier[2].time;
        }
        if (u[n].ddl - u[n].tier[1].time < min_time) {
            min_time = u[n].ddl - u[n].tier[1].time;
        }
    }

    // Pre-calculate the distance between servers and users
    for (int m = 1; m <= M; m++) {
        for (int n = 1; n <= N; n++) {
            s_distance.at(m).at(n) = calc_distance(s[m].x, s[m].y, u[n].x, u[n].y);
        }
    }
    // Pre-calculate the distance between users and relays & relays and servers
    for (int l = 1; l <= L; l++) {
        for (int n = 1; n <= N; n++) {
            ur_distance.at(l).at(n) = calc_distance(s[l + M].x, s[l + M].y, u[n].x, u[n].y);
        }
        for (int m = 1; m <= M; m++) {
            rs_distance.at(l).at(m) = calc_distance(s[l + M].x, s[l + M].y, s[m].x, s[m].y);
        }
    }

    // Pre-calculate the minimum timeslot required for each user-server pair
    for (int n = 1; n <= N; n++) {
        for (int m = 1; m <= M; m++) {
            timing.at(n).at(m) = new TIMING();

            int best_relay = -1;
            int best_T = INT_MAX;
            int best_ur_time = -1;
            int best_rs_time = -1;

            // Test direct connection
            double snr_result = snr(m, n);
            if (snr_result < beta) {
                continue;
            } else {
                int used_T = static_cast<int>(std::ceil(trans_time(u[n].data, snr_result) / (X * z)));
                if (used_T < best_T) {
                    best_relay = 0;
                    best_T = used_T;
                    best_ur_time = -1;
                    best_rs_time = -1;
                }
            }

            // Test relay connection
            for (int l = 1; l <= L; l++) {
                double snr_ur_result = snr_ur(l, n);
                double snr_rs_result = snr_rs(l, m);
                if (snr_ur_result < beta || snr_rs_result < beta) {
                    continue;
                } else {
                    int ur_time = static_cast<int>(std::ceil(trans_time(u[n].data, snr_ur_result) / (X * z)));
                    int rs_time = static_cast<int>(std::ceil(trans_time(u[n].data, snr_rs_result) / (X * z)));
                    int used_T = ur_time + rs_time;
                    if (used_T < best_T) {
                        best_relay = l;
                        best_T = used_T;
                        best_ur_time = ur_time;
                        best_rs_time = rs_time;
                    }
                }
            }

            timing.at(n).at(m)->relay = best_relay;
            timing.at(n).at(m)->T = best_T;
            timing.at(n).at(m)->ur_time = best_ur_time;
            timing.at(n).at(m)->rs_time = best_rs_time;
        }
    }

    table_size = 1;
    for (int m = 1; m <= M + L; m++) {
        if (flag == 0) {
            table_size = table_size * (1 + s[m].cpu) * (1 + s[m].ram);
        } else {
            table_size = table_size * (1 + lambda) * (1 + lambda);
        }
    }
    table_size = table_size * (1 + T) * (1 + N);
    opt = new OPT[table_size];

    // Find the optimal X
    // Lower bound should satisfy the minimum time requirement
    // Upper bound should not exceed the maximum time requirement
//    int X_lb = std::ceil(min_time / (X * z));
    int X_ub = std::ceil(max_time / (X * z));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << std::format("Preprocessing took {} seconds\n",
                             std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0) << std::endl;
    std::cout << std::format("Table size: {}\n", table_size) << std::endl;

    int best_reward = 0, best_X = 0;
    std::vector<std::vector<int>> best_results;

    // Iterate through the possible X values to determine which one gives the best outcome
    for (int x = 3; x <= 3; x++) {
        X = x;
        // Update timing info after x changes
        for (int n = 1; n <= N; n++) {
            for (int m = 1; m <= M; m++) {
                // Using relay
                int l = timing.at(n).at(m)->relay;
                if (l > 0) {
                    double snr_ur_result = snr_ur(l, n);
                    double snr_rs_result = snr_rs(l, m);
                    int ur_time = static_cast<int>(std::ceil(trans_time(u[n].data, snr_ur_result) / (X * z)));
                    int rs_time = static_cast<int>(std::ceil(trans_time(u[n].data, snr_rs_result) / (X * z)));
                    int used_T = ur_time + rs_time;
                    timing.at(n).at(m)->T = used_T;
                    timing.at(n).at(m)->ur_time = ur_time;
                    timing.at(n).at(m)->rs_time = rs_time;
                } else if(l == 0) { // Direct connection
                    double snr_result = snr(m, n);
                    int used_T = static_cast<int>(std::ceil(trans_time(u[n].data, snr_result) / (X * z)));
                    timing.at(n).at(m)->T = used_T;
                }
            }
        }

        begin = std::chrono::steady_clock::now();
        std::cout << "Calculating for X = " << X << std::endl;
        dp(flag);
        end = std::chrono::steady_clock::now();
        auto time_delta = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;

        fs::path p = argv[1];
        auto out_path = p.parent_path() / std::format("output_{}_X{}_T{}_L{}.txt",
                                                      mode_str, x, T, lambda);

        auto solution = trace_solution(opt, flag, N);

        if (solution.back().back() > best_reward) {
            best_reward = solution.back().back();
            best_X = x;
            best_results = solution;
        }

        print_to_file(out_path.string(), solution, time_delta);
        std::cout << std::format("Time taken: {} seconds", time_delta) << std::endl;
        std::cout << std::format("Optimal value: {}\n", solution.back().back()) << std::endl;
        std::cout << std::endl;

    }

    std::string tc_num = std::regex_replace(
            p.parent_path().stem().string(),
            std::regex("[^0-9]*([0-9]+).*"),
            std::string("$1")
    );
    result_to_csv(csv_file, mode_str, std::stoi(tc_num), best_X, lambda, best_reward,
                  std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0);

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
