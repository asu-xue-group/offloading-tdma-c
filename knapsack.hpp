#ifndef TDMA_KNAPSACK_KNAPSACK_HPP
#define TDMA_KNAPSACK_KNAPSACK_HPP

#include <algorithm>
#include <format>
#include <iostream>
#include <vector>
#include <tuple>
#include "global.h"
#include "structs.h"
#include "subs.hpp"
#include "output.hpp"
#include "graph.hpp"


extern OPT *opt;
extern long long table_size;
extern Graph graph;
extern std::vector<std::vector<OPT_PATH*>> opt_path;

void cartesian_recurse(std::vector<std::vector<int>> &accum, std::vector<int> stack,
                       std::vector<std::vector<int>> sequences, int index) {
    std::vector<int> sequence = sequences[index];
    for (int i: sequence) {
        stack.push_back(i);
        if (index == 0)
            accum.push_back(stack);
        else
            cartesian_recurse(accum, stack, sequences, index - 1);
        stack.pop_back();
    }
}

std::vector<std::vector<int>> cartesian_product(const std::vector<std::vector<int>> &sequences) {
    std::vector<std::vector<int>> accum;
    std::vector<int> stack;
    if (!sequences.empty())
        cartesian_recurse(accum, stack, sequences, static_cast<int>(sequences.size()) - 1);
    return accum;
}


std::tuple<float, int, int, int> calc_opt(int n, int t, const std::vector<int> &combo, int mode) {
    auto val = 0.0f;
    if (n > 1) {
        val = opt[get_idx(n - 1, t, combo, mode)].reward;
    }
    int m_opt = 0;
    int k_opt = 0;
    int slot_opt = 0;

    // Edge case where everything is zero
    if (T == 0 && std::all_of(combo.begin() + 1, combo.end(), [](int i) { return i == 0; })) {
        return {0, 0, 1, 0};
    }

    // Iterate over all servers
    for (int m = 0; m <= M + L; m++) {
        // Iterate over offloading tiers
        for (int k = 1; k <= K; k++) {
            auto prev_opt = 0.0f;
            auto required_T = 0;
            std::vector<std::string> path;
            std::vector<int> timeslots;
            // Local processing
            if (m == 0) {
                if (u[n].cpu < static_cast<float>(u[n].tier[k].cpu) || u[n].ram < static_cast<float>(u[n].tier[k].ram)) {
                    continue;
                }
                if (n > 1) {
                    prev_opt = opt[get_idx(n - 1, t, combo, mode)].reward;
                }
            } else {
                // Update the timeslot information on the graph
                graph.update_timeslot(u[n].data, u[n].ddl - u[n].tier[k].time);
                auto result = graph.shortest_path(u[n].name, s[m].name);
                if (result == std::nullopt) continue;
                std::tie(path, timeslots, required_T) = result.value();
                // Check the remaining cpu, ram, timeslot
                auto new_combo = combo;
                update_combo(new_combo, n, m, k, mode);
                // Check the timeslot
                auto new_t = t - required_T;

                // Check if resources are available
                if (new_combo[2 * m - 1] < 0 || new_combo[2 * m] < 0 || new_t < 0) {
                    continue;
                }
                if (n > 1) {
                    prev_opt = opt[get_idx(n - 1, new_t, new_combo, mode)].reward;
                }
            }

            auto reward = static_cast<float>(u[n].tier[k].reward) + prev_opt;

            if (reward > val) {
                val = reward;
                m_opt = m;
                k_opt = k;
                slot_opt = required_T;
                opt_path.at(n).at(k)->path = path;
                opt_path.at(n).at(k)->timeslots = timeslots;
            }

        }
    }
    
    return {val, m_opt, k_opt, slot_opt};
}


void dp(int mode) {
    // Mode 0: normal DP
    // Mode 1: "relaxed" DP
    // Mode -1: "restricted" DP
    // Pre-calculate the Cartesian product of the server CPU and RAM combinations
    std::vector<std::vector<int>> server_cpu_ram;

    // Relay and server together
    for (int m = M + L; m >= 1; m--) {
        std::vector<int> ram;
        std::vector<int> cpu;

        for (int rr = 0; rr <= s[m].ram; rr++) {
            ram.push_back(rr);
        }
        for (int c = 0; c <= s[m].cpu; c++) {
            cpu.push_back(c);
        }

        server_cpu_ram.push_back(ram);
        server_cpu_ram.push_back(cpu);
    }


    // A placeholder for the zero index
    server_cpu_ram.push_back({0});
    auto combos = cartesian_product(server_cpu_ram);

    for (int n = 1; n <= N; n++) {
        std::cout << "Calculating for user " << n << std::endl;
        for (int t = 0; t <= T; t++) {
            for (const auto &cc: combos) {
                auto next_idx = get_idx(n, t, cc, mode);
                if (next_idx >= table_size) {
                    std::cerr << "Table size exceeded: accessing " << next_idx << " out of " << table_size << std::endl;
//                    exit(1);
                }
                auto [reward, m_opt, k_opt, slot_opt] = calc_opt(n, t, cc, mode);
                auto solution = mux_solution(m_opt, k_opt);
                opt[next_idx].solution = solution;
                opt[next_idx].slot = static_cast<unsigned char>(slot_opt);
                opt[next_idx].reward = reward;
            }
        }

//        std::cout << "Tracing solution for user " << n << std::endl;
        auto curr_solution = trace_solution(opt, mode, n);

#ifdef print_solution
        print_results(curr_solution, n);
#endif
    }
}

#endif //TDMA_KNAPSACK_KNAPSACK_HPP
