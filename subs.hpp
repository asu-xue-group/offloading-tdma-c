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

double calc_snr(double distance) {
    return Pmax / (noise * std::pow(distance, alpha));
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

// ---------- packing ----------
inline uint16_t pack_meta(uint8_t m, uint8_t k, uint8_t slot)
{
    return   (m    & 0x0F)           |        // bits 0-3
            ((k    & 0x03) << 4 )    |        // bits 4-5
            ((slot & 0x3F) << 6 );            // bits 6-11
            /* bits 12-15 left 0 (reserved) */
}

// ---------- unpacking ----------
inline void unpack_meta(uint16_t meta,
                        uint8_t& m, uint8_t& k, uint8_t& slot)
{
    m    =  meta        & 0x0F;
    k    = (meta >> 4)  & 0x03;
    slot = (meta >> 6)  & 0x3F;
}


long long get_idx(int n, int t, const std::vector<int> &combo, int mode) {
    int C[M + L + 1];
    int R[M + L + 1];

    for (int m = 1; m <= M + L; m++) {
        C[m] = combo[2 * m - 1];
        R[m] = combo[2 * m];
    }

    return indexue(n, t, C, R);
}

// Exponential decay function
double calc_reward(int n, int k, double delay) {
    return u[n].tier[k].reward * std::pow(M_E, -decay * delay);
}


#endif