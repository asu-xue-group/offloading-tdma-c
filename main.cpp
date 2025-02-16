#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <vector>
#include "global.h"
#include "knapsack.hpp"
#include "output.hpp"
#include "structs.h"
#include "subs.hpp"


//#define debug
//#define print_solution

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
    FILE *fp1;         // input file
    int returnV, flag;

    // Check commandline arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ifile> <flag>\n", argv[0]);
        exit(1);
    }
    flag = std::stoi(argv[2]);

    fp1 = fopen(argv[1], "r");
    if (!fp1) {
        fprintf(stderr, "Error: cannot open file %s\n", argv[1]);
        exit(0);
    }
    // Read in the size of the test case
    returnV = fscanf(fp1, "%d%d%d%d", &K, &M, &L, &N);
    if (returnV != 4) {
        fprintf(stderr, "wrong input\n");
        exit(0);
    }

    std::cout << std::format("Running test case {} with flag {}, T={}, X={}, z={}\n",
                             argv[1], flag, T, X, z) << std::endl;

    // Initialize the data structures
    // Here relays are considered as servers
    s = (SERVER *) calloc(M + L + 1, sizeof(SERVER));
//    r = (SERVER *) calloc(L + 1, sizeof(SERVER));
    u = (USER *) calloc(N + 1, sizeof(USER));
    s_distance = std::vector<std::vector<double>>(M + 1);
    ur_distance = std::vector<std::vector<double>>(L + 1);
    rs_distance = std::vector<std::vector<double>>(L + 1);
    timing = std::vector<std::vector<TIMING *>>(N + 1);
    for (int i = 0; i <= N; i++) {
        timing.at(i) = std::vector<TIMING *>(M + 1);
    }

    for (int m = 1; m <= M + L; m++) {
        fscanf(fp1, "%d", &s[m].index);
        fscanf(fp1, "%f", &s[m].x);
        fscanf(fp1, "%f", &s[m].y);
        fscanf(fp1, "%d", &s[m].cpu);
        fscanf(fp1, "%d", &s[m].ram);
        if (m <= M) {
            s_distance.at(m) = std::vector<double>(N + 1);
        } else {
            ur_distance.at(m - M) = std::vector<double>(N + 1);
            rs_distance.at(m - M) = std::vector<double>(M + 1);
        }
    }

    for (int n = 1; n <= N; n++) {
        fscanf(fp1, "%d", &u[n].index);
        fscanf(fp1, "%f", &u[n].x);
        fscanf(fp1, "%f", &u[n].y);
        fscanf(fp1, "%f", &u[n].ddl);
        fscanf(fp1, "%f", &u[n].data);

        fscanf(fp1, "%d", &u[n].tier[1].tier);
        fscanf(fp1, "%d", &u[n].tier[1].cpu);
        fscanf(fp1, "%d", &u[n].tier[1].ram);
        fscanf(fp1, "%d", &u[n].tier[1].reward);
        fscanf(fp1, "%f", &u[n].tier[1].time);

        fscanf(fp1, "%d", &u[n].tier[2].tier);
        fscanf(fp1, "%d", &u[n].tier[2].cpu);
        fscanf(fp1, "%d", &u[n].tier[2].ram);
        fscanf(fp1, "%d", &u[n].tier[2].reward);
        fscanf(fp1, "%f", &u[n].tier[2].time);
    }
    fclose(fp1);

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
            timing.at(n).at(m) = (TIMING *) calloc(1, sizeof(TIMING));

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

    opt = (OPT *) calloc(table_size, sizeof(OPT));
    // Run the dynamic programming algorithm
    dp(flag);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto time_delta = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;

    fs::path p = argv[1];
    std::string mode_str = "original";
    if (flag == 1) {
        mode_str = "relaxed";
    } else if (flag == -1) {
        mode_str = "restricted";
    }
    auto out_path = p.parent_path() / std::format("output_{}_T{}_L{}_{}.txt",
                                                  mode_str, T, lambda, static_cast<int>(total));

    auto solution = trace_solution(opt, flag, N);
    print_to_file(out_path.string(), solution, time_delta);

#ifdef print_solution
    print_results(solution, N);
#endif

    std::cout << "Time taken: " << time_delta << " seconds\n";
    std::cout << "Table size: " << table_size << std::endl;

    free(s);
    free(u);
    free(opt);

    return 0;
}
