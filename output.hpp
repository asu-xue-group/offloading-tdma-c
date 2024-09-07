#ifndef TDMA_KNAPSACK_OUTPUT_HPP
#define TDMA_KNAPSACK_OUTPUT_HPP

#include <vector>
#include <array>
#include <string>
#include "subs.hpp"


void print_to_file(const std::string &filename, const std::vector<std::vector<int>> &solution, double time) {
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file %s\n", filename.c_str());
        exit(0);
    }

    fprintf(fp, "T=%d, lambda=%d\n", T, lambda);
    fprintf(fp, "Optimal value: %d\n", solution.back().back());
    fprintf(fp, "Optimal solution:\n");
    for (const auto &o: solution) {
        if (o[1] == 0) {
            fprintf(fp, "User %d is not assigned to any server\n", o[0]);
        } else {
            fprintf(fp, "User %d is assigned to server %d with algo %d, and was assigned %d time slots\n",
                    o[0], o[1], o[2], o[3]);
        }
    }
    fprintf(fp, "Time taken: %.4f seconds\n", time);

    fclose(fp);
}


void print_results(const std::vector<std::vector<int>> &solution, int n) {
    printf("n=%d, opt=%d\n", n, solution.back().back());
    printf("Optimal solution:\n");
    for (const auto &o: solution) {
        printf("n=%d, m=%d, k=%d, slot=%d, reward=%d\n", o[0], o[1], o[2], o[3], o[4]);
    }
    printf("\n");
}


std::vector<std::vector<int>> trace_solution(OPT *opt, int flag, int num_user) {
    std::vector<std::vector<int>> solution(num_user);
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

    for (int n = num_user; n >= 1; n--) {
        auto index = get_idx(n, curr_t, curr_combo, flag);
        auto sol = opt[index].solution;
        auto reward = opt[index].reward;
        auto [slot_opt, m_opt, k_opt] = demux_solution(sol);
        solution.at(n-1) = std::vector<int>{n, m_opt, k_opt, slot_opt, reward};
        if (m_opt == 0) {
            continue;
        }
        curr_t -= slot_opt;
        update_combo(curr_combo, n, m_opt, k_opt, flag);
    }

    return solution;
}

#endif //TDMA_KNAPSACK_OUTPUT_HPP
