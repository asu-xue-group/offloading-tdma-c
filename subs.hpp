#ifndef MY_SUBS_HPP
#define MY_SUBS_HPP 1


#include <cmath>
#include <tuple>
#include "global.h"
#include "structs.h"


extern SERVER *s;
extern USER *u;
extern std::vector<std::vector<double>> s_distance, ur_distance, rs_distance;
extern int K, M, N, L;

long long indexue(int n, int t, const int *C, const int *R) {
    long long temp = 0;
    long long multiplier = (1 + T);

    for (int m = 1; m <= M + L; m++) {
        multiplier *= (1 + s[m].cpu);
        multiplier *= (1 + s[m].ram);
    }

    temp += n * multiplier;

    multiplier /= (1 + T);
    temp += t * multiplier;

    for (int m = 1; m <= M + L; m++) {
        multiplier /= (1 + s[m].cpu);
        temp += C[m] * multiplier;
        multiplier /= (1 + s[m].ram);
        temp += R[m] * multiplier;
    }

    return temp;
}

double calc_distance(float x1, float y1, float x2, float y2) {
    return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
}

double snr(int m, int n) {
    return Pmax / (noise * std::pow(s_distance.at(m).at(n), alpha));
}

double snr_ur(int l, int n) {
    return Pmax / (noise * std::pow(ur_distance.at(l).at(n), alpha));
}

double snr_rs(int l, int m) {
    return Pmax / (noise * std::pow(rs_distance.at(l).at(m), alpha));
}

double trans_time(double data, double snr) {
    return data / (bandwidth * std::log2(1 + snr));
}

void update_combo(std::vector<int> &combo, int n, int m, int k, int mode) {
    if (!s[m].cpu_scaled) {
        combo[2 * m - 1] -= u[n].tier[k].cpu;
    } else {
        if (mode == 1) {
            combo[2 * m - 1] -= static_cast<int>(std::floor(lambda * u[n].tier[k].cpu / static_cast<double>(s[m].cpu_orig)));
        } else if (mode == -1) {
            combo[2 * m - 1] -= static_cast<int>(std::ceil(lambda * u[n].tier[k].cpu / static_cast<double>(s[m].cpu_orig)));
        }
    }
    if (!s[m].ram_scaled) {
        combo[2 * m] -= u[n].tier[k].ram;
    } else {
        if (mode == 1) {
            combo[2 * m] -= static_cast<int>(std::floor(lambda * u[n].tier[k].ram / static_cast<double>(s[m].ram_orig)));
        } else if (mode == -1) {
            combo[2 * m] -= static_cast<int>(std::ceil(lambda * u[n].tier[k].ram / static_cast<double>(s[m].ram_orig)));
        }
    }
}

unsigned char mux_solution(int server, int tier) {
    if (server > 55) {
        std::cerr << "Server number is " << server << " out of max 55" << std::endl;
    }
    if (tier > 2) {
        std::cerr << "Tier number is " << tier << " out of max 2" << std::endl;
    }
    return tier * 100 + server;
}

std::tuple<int, int> demux_solution(unsigned char solution) {
    int tier = solution / 100;
    int server = solution % 100;
    return {server, tier};
}
//
//unsigned short mux_solution2(int reward, int slot) {
//    if (reward > 655) {
//        std::cerr << "Reward number is " << reward << " out of max 655" << std::endl;
//    }
//    if (slot > 35) {
//        std::cerr << "Slot number is " << slot << " out of max 35" << std::endl;
//    }
//    return reward * 100 + slot;
//}
//
//std::tuple<int, int> demux_solution2(unsigned short solution) {
//    int reward = solution / 100;
//    int slot = solution % 100;
//    return {reward, slot};
//}

long long get_idx(int n, int t, const std::vector<int> &combo, int mode) {
    int C[M + L + 1];
    int R[M + L + 1];

    for (int m = 1; m <= M + L; m++) {
        C[m] = combo[2 * m - 1];
        R[m] = combo[2 * m];
    }

    return indexue(n, t, C, R);
}

#endif