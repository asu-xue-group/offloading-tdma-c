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
std::vector<std::vector<double>> distance;
int K, M, N;
long long table_size;


int main(int argc, char **argv) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    FILE *fp1;         // input file
    int returnV, flag;

    // Check commandline arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <ifile> <flag> <T> <t0>\n", argv[0]);
        exit(1);
    }
    flag = std::stoi(argv[2]);
    T = std::stoi(argv[3]);
    total = std::stof(argv[4]);

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

    std::cout << std::format("Running test case {} with flag {}, T={}, t0={}\n",
                             argv[1], flag, T, total) << std::endl;


    s = (SERVER *) calloc(M + 1, sizeof(SERVER));
    u = (USER *) calloc(N + 1, sizeof(USER));
    distance = std::vector<std::vector<double>>(M + 1);

    for (int m = 1; m <= M; m++) {
        fscanf(fp1, "%d", &s[m].index);
        fscanf(fp1, "%f", &s[m].x);
        fscanf(fp1, "%f", &s[m].y);
        fscanf(fp1, "%d", &s[m].cpu);
        fscanf(fp1, "%d", &s[m].ram);
        distance.at(m) = std::vector<double>(N + 1);
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
            distance.at(m).at(n) = calc_distance(s[m].x, s[m].y, u[n].x, u[n].y);
        }
    }

    table_size = 1;
    for (int m = 1; m <= M; m++) {
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
