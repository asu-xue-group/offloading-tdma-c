#ifndef TDMA_KNAPSACK_OUTPUT_HPP
#define TDMA_KNAPSACK_OUTPUT_HPP

#include <vector>
#include <array>
#include <string>
#include <variant>
#include "subs.hpp"

extern std::vector<std::vector<TIMING *>> timing;

void print_to_file(const std::string &filename, const std::vector<std::vector<std::variant<int, float>>> &solution, double time) {
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file %s\n", filename.c_str());
        exit(0);
    }

    // o[0] = user index, o[1] = server index, o[2] = algo index, o[3] = timeslots allocated, o[4] = reward
    fprintf(fp, "T=%d, lambda=%d\n", T, lambda);
    fprintf(fp, "Optimal value: %f\n", std::get<float>(solution.back().back()));
    fprintf(fp, "Optimal solution:\n");
    for (const auto &o: solution) {
        auto o0 = std::get<int>(o[0]);
        auto o1 = std::get<int>(o[1]);
        auto o2 = std::get<int>(o[2]);
        auto o3 = std::get<int>(o[3]);

        auto index_str = std::format("{}_{}_{}", o0, o1, o2);
        std::string penalty_string;
//        if (penalized.contains(index_str)) {
//            auto delay = penalized.at(index_str).at(0);
//            auto reward_pct = penalized.at(index_str).at(1);
//            penalty_string = std::format("(delay: {:.2f}, reward: {:.0f}%)", delay, reward_pct * 100);
//        }
        if (o1 == 0) {
            if (o2 == 0) {
                fprintf(fp, "User %d is not scheduled\n", o0);
            } else {
                fprintf(fp, "User %d is assigned to local processing with algo %d\n", o0, o2);
            }
        } else if (o1 <= M) {
//            if (timing.at(o0).at(o1)->relay == 0) {
//                fprintf(fp,
//                        "User %d is assigned to server %d with algo %d (direct connection), and is assigned %d time slots %s\n",
//                        o0, o1, o2, o3, penalty_string.c_str());
//
//            } else {
//                fprintf(fp,
//                        "User %d is assigned to server %d with algo %d, via relay %d, and is assigned %d time slots %s\n",
//                        o0, o1, o2, timing.at(o0).at(o1)->relay, o3, penalty_string.c_str());
//            }
        } else {
            fprintf(fp, "User %d is assigned to relay %d with algo %d, and is assigned %d time slots %s\n",
                    o0, o1 - M, o2, o3, penalty_string.c_str());
        }
    }
    fprintf(fp, "Time taken: %.4f seconds\n", time);

    fclose(fp);
}

void result_to_csv(const std::filesystem::path& filename, const std::string& flag, int tc_num, int x, int _lambda, float reward, double time, long long table_size) {
    FILE *fp = fopen(filename.string().c_str(), "a");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file %s\n", filename.string().c_str());
        exit(0);
    }

    fprintf(fp, "%s,%d,%d,%d,%.4f,%.4f,%lld\n", flag.c_str(), tc_num, x, _lambda, reward, time, table_size);
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


std::vector<std::vector<std::variant<int, float>>> trace_solution(OPT *opt, int flag, int num_user) {
    std::vector<std::vector<std::variant<int, float>>> solution(num_user);
    int curr_t = T;
    auto curr_combo = std::vector<int>();
    curr_combo.push_back(0);
    for (int m = 1; m <= M + L; m++) {
        curr_combo.push_back(s[m].cpu);
        curr_combo.push_back(s[m].ram);
    }

    for (int n = num_user; n >= 1; n--) {
        auto index = get_idx(n, curr_t, curr_combo, flag);
        auto sol = opt[index].solution;
        auto reward = opt[index].reward;
        auto slot_opt = opt[index].slot;
        auto [m_opt, k_opt] = demux_solution(sol);
        solution.at(n - 1) = std::vector<std::variant<int, float>>{n, m_opt, k_opt, slot_opt, reward};
        if (m_opt == 0) {
            continue;
        }
        curr_t -= slot_opt;
        update_combo(curr_combo, n, m_opt, k_opt, flag);
    }

    return solution;
}

#endif //TDMA_KNAPSACK_OUTPUT_HPP
