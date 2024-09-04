#include <cstdio>
#include <cstdlib>
#include "global.h"
#include "structs.h"
#include "subs.hpp"
#include "knapsack.hpp"
#include <vector>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>

#ifdef __linux__
#include "sys/types.h"
#include "sys/sysinfo.h"
#elif _WIN32

#include "windows.h"
#include "output.hpp"

#endif

namespace fs = std::filesystem;
SERVER *s;
USER *u;
OPT *opt;
int K, M, N, T;


int main(int argc, char **argv) {
#ifdef __linux__
    struct sysinfo memInfo;
    sysinfo (&memInfo);
#elif _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
#endif
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    FILE *fp1;         // input file
    int returnV, flag;

    int table_size;

    // Check commandline arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <ifile> <flag> <T>\n", argv[0]);
        exit(1);
    }
    flag = atoi(argv[2]);
    T = atoi(argv[3]);

    fp1 = fopen(argv[1], "r");
    if (!fp1) {
        fprintf(stderr, "Error: cannot open file %s\n", argv[1]);
        exit(0);
    }
    // Read in the size of the test case
    returnV = fscanf(fp1, "%d%d%d", &K, &M, &N);
    if (returnV != 3) {
        fprintf(stderr, "wrong input\n");
        exit(0);
    }
//    fprintf(stdout, "K=%d, M=%d, N=%d\n", K, M, N);
//    fprintf(stdout, "sizeof(OPT)=%d\n", sizeof(OPT));
    std::cout << "Running test case " << argv[1] << " with flag " << flag << std::endl;

    s = (SERVER *) calloc(M + 1, sizeof(SERVER));
    u = (USER *) calloc(N + 1, sizeof(USER));

    for (int m = 1; m <= M; m++) {
        fscanf(fp1, "%d", &s[m].index);
        fscanf(fp1, "%f", &s[m].x);
        fscanf(fp1, "%f", &s[m].y);
        fscanf(fp1, "%d", &s[m].cpu);
        fscanf(fp1, "%d", &s[m].ram);
        s[m].distance.push_back(0.0);
    }
//    fprintf(stdout, "servers read in\n");

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
//    fprintf(stdout, "users read in\n");

    // Pre-calculate the distance between servers and users
    for (int m = 1; m <= M; m++) {
        for (int n = 1; n <= N; n++) {
            s[m].distance.push_back(calc_distance(s[m].x, s[m].y, u[n].x, u[n].y));
        }
    }

    table_size = 1;
//    fprintf(stdout, "table_size=%d\n", table_size);
    for (int m = 1; m <= M; m++) {
//        fprintf(stdout, "1 + s[%d].cpu)=%d\n", m, 1 + s[m].cpu);
//        fprintf(stdout, "1 + s[%d].ram)=%d\n", m, 1 + s[m].ram);
        if (flag == 0) {
            table_size = table_size * (1 + s[m].cpu) * (1 + s[m].ram);
        } else {
            table_size = table_size * (1 + lambda) * (1 + lambda);
        }
//        fprintf(stdout, "m=%d, table_size=%d\n", m, table_size);
    }
    table_size = table_size * (1 + T) * (1 + N);

//    fprintf(stdout, "K=%d, M=%d, N=%d, T=%d, table_size=%d\n", K, M, N, T, table_size);
//    fprintf(stdout, "table_size=%d\n", table_size);
    opt = (OPT *) calloc(table_size, sizeof(OPT));

    // Run the dynamic programming algorithm
    dp(flag);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::vector<std::array<int, 4>> solution;
    int curr_t = T;
    auto curr_combo = std::vector<int>();
    curr_combo.push_back(0);
    if (flag == 0) {
        for (int m = 1; m <= M; m++) {
            curr_combo.push_back(s[m].cpu);
            curr_combo.push_back(s[m].ram);
        }
    } else {
        for (int m = 1; m <= M; m++) {
            curr_combo.push_back(lambda);
            curr_combo.push_back(lambda);
        }
    }

    int max_reward = opt[get_idx(N, T, curr_combo, flag)].reward;
    for (int n = N; n >= 1; n--) {
        auto [sol, reward] = opt[get_idx(n, curr_t, curr_combo, flag)];
        auto [slot_opt, m_opt, k_opt] = demux_solution(sol);
        solution.push_back({n, m_opt, k_opt, slot_opt});
        curr_t -= slot_opt;
        update_combo(curr_combo, n, m_opt, k_opt, flag);
    }
    std::reverse(solution.begin(), solution.end());

    auto time_delta = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;
    double total_mem = 0.0;
#ifdef __linux__
    long long mem_byte = memInfo.totalram;
    total_mem = mem_byte / 1000000.0;
#elif _WIN32
    DWORDLONG mem_bit = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    total_mem = mem_bit / 8000000000.0;
#endif
    fs::path p = argv[1];
    std::string mode_str = "original";
    if (flag == 1) {
        mode_str = "relaxed";
    } else if (flag == -1) {
        mode_str = "restricted";
    }
    auto out_path = p.parent_path() / std::format("output_{}_T{}_L{}.txt", mode_str, T, lambda);
    print_to_file(out_path.string(), solution, max_reward, time_delta, total_mem);

    free(s);
    free(u);
    free(opt);

    std::cout << "Finished. Memory used: " << total_mem << " MB\n";
    std::cout << "Time taken: " << time_delta << " seconds\n";

    return 0;
}
