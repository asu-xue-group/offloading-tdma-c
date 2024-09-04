#ifndef MY_SUBS_HPP
#define MY_SUBS_HPP 1

#include "structs.h"
#include "global.h"
#include <cmath>
#include <tuple>


extern SERVER *s;
extern USER *u;
extern int K, M, N, T;

long indexue(int n, int t, const int *C, const int *R, int mode) {
    long temp = 0;
    long multiplier = (1 + T);

    if (mode == 0) {
        for (int m = 1; m <= M; m++) {
            multiplier = multiplier * (1 + s[m].cpu);
            multiplier = multiplier * (1 + s[m].ram);
        }
    } else {
        for (int m = 1; m <= M; m++) {
            multiplier = multiplier * (1 + lambda) * (1 + lambda);
        }
    }

    temp += n * multiplier;

    multiplier /= (1 + T);
    temp += t * multiplier;

    if (mode == 0) {
        for (int m = 1; m <= M; m++) {
            multiplier /= (1 + s[m].cpu);
            temp += C[m] * multiplier;
            multiplier /= (1 + s[m].ram);
            temp += R[m] * multiplier;
        }
    } else {
        for (int m = 1; m <= M; m++) {
            multiplier /= (1 + lambda);
            temp += C[m] * multiplier;
            multiplier /= (1 + lambda);
            temp += R[m] * multiplier;
        }
    }

    return temp;
};

double calc_distance(float x1, float y1, float x2, float y2) {
    return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
}

double snr(int m, int n) {
    return Pmax / (noise * std::pow(s[m].distance[n], alpha));
}

void update_combo(std::vector<int> &combo, int n, int m, int k, int mode) {
    if (mode == 0) {
        combo[2 * m - 1] -= u[n].tier[k].cpu;
        combo[2 * m] -= u[n].tier[k].ram;
    } else if (mode == 1) {
        combo[2 * m - 1] -= static_cast<int>(std::floor(lambda * u[n].tier[k].cpu / static_cast<double>(s[m].cpu)));
        combo[2 * m] -= static_cast<int>(std::floor(lambda * u[n].tier[k].ram / static_cast<double>(s[m].ram)));
    } else if (mode == -1) {
        combo[2 * m - 1] -= static_cast<int>(std::ceil(lambda * u[n].tier[k].cpu / static_cast<double>(s[m].cpu)));
        combo[2 * m] -= static_cast<int>(std::ceil(lambda * u[n].tier[k].ram / static_cast<double>(s[m].ram)));
    }
}

unsigned short mux_solution(int slot, int server, int tier) {
    return slot * 1000 + server * 10 + tier;
}

std::tuple<int, int, int> demux_solution(unsigned short solution) {
    int slot = solution / 1000;
    int server = (solution % 1000) / 10;
    int tier = solution % 10;
    return {slot, server, tier};
}

#endif