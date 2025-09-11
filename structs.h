#ifndef _structs_h
#define _structs_h 1


#include <vector>
#include <array>
#include <cstdint>

struct SERVER {
    bool cpu_scaled;          // whether the CPU is scaled
    bool ram_scaled;          // whether the RAM is scaled
    bool relay;             // whether this is a relay
    int index;                // index of the server, starting from 1...
    int cpu;                  // number of CPU cores
    int cpu_orig;             // original number of CPU cores (before scaling)
    int ram;                  // size of memory
    int ram_orig;             // original size of memory (before scaling)
    float x;                    // x coordinate of the server
    float y;                    // y coordinate of the server
    std::string name;       // name of the server
};


struct TIER {
    int tier;       // tier in {1, 2}. This may not be needed, but we will keep it.
    int cpu;        // cpu requirement
    int ram;        // ram requirement
    int reward;     // corresponding reward
    float time;       // execution time
};

struct USER {
    int index;      // index of the user, starting from 1...
    float cpu;         // locally available CPU on user
    float ram;         // locally available RAM on user
    float x;          // x coordinate of the user
    float y;          // y coordinate of the user
    float ddl;        // deadline
    float data;       // data size, in MB
    std::string name; // name of the user
    std::vector<float> fail_prob;  // failure probability
    TIER tier[3];    // task requirement and reward: tier[1] and tier[2], wasting tier[0]...
};


// struct OPT {
// //    unsigned char mn;         // this user is assigned to server m, mn=0 means we skip this user.
// //    unsigned char kn;         // this user is assigned to accuracy k
//     unsigned char solution;   // multiplexed server and accuracy
//     unsigned char slot;      // the number of time slots assigned to this user
//     float reward;     // the corresponding optimal value
// };

struct OPT_PATH {
    int required_T = -1;
    int X_n = -1;
    std::vector<std::string> path;
    std::vector<int> timeslots;
};

struct OPT {
    std::array<uint8_t, 5> bytes;   // [meta_lo meta_hi | q0 q1 q2]
};

inline uint32_t q_from_float(float reward) {
    if (reward <= 0.0f) return 0;
    double q = std::llround(static_cast<double>(reward) * 10000.0);
    if (q > 0xFFFFFF) q = 0xFFFFFF;   // clamp to 24-bit
    return static_cast<uint32_t>(q);
}
inline float float_from_q(uint32_t q) {
    return static_cast<float>(q) * 0.0001f;
}

// ---- pack/unpack the 16-bit meta ----
inline uint16_t pack_meta(uint8_t m, uint8_t k, uint8_t slot) {
    return (m & 0x0F) | ((k & 0x03) << 4) | ((slot & 0x7F) << 6);
}
inline void unpack_meta(uint16_t meta, uint8_t& m, uint8_t& k, uint8_t& slot) {
    m    =  meta        & 0x0F;
    k    = (meta >> 4)  & 0x03;
    slot = (meta >> 6)  & 0x7F;
}

inline OPT pack_opt(uint8_t m, uint8_t k, uint8_t slot, float reward) {
    OPT p{};
    const uint16_t meta = pack_meta(m,k,slot);
    const uint32_t q    = q_from_float(reward);   // 24-bit payload

    p.bytes[0] = static_cast<uint8_t>( meta      & 0xFF);
    p.bytes[1] = static_cast<uint8_t>((meta >> 8) & 0xFF);
    p.bytes[2] = static_cast<uint8_t>( q         & 0xFF);
    p.bytes[3] = static_cast<uint8_t>((q  >> 8)  & 0xFF);
    p.bytes[4] = static_cast<uint8_t>((q  >> 16) & 0xFF);
    return p;
}

inline void unpack_opt(const OPT& p, uint8_t& m, uint8_t& k,
                        uint8_t& slot, float& reward) {
    const uint16_t meta =
        (static_cast<uint16_t>(p.bytes[0])      ) |
        (static_cast<uint16_t>(p.bytes[1]) <<  8);
    const uint32_t q =
        (static_cast<uint32_t>(p.bytes[2])      ) |
        (static_cast<uint32_t>(p.bytes[3]) <<  8) |
        (static_cast<uint32_t>(p.bytes[4]) << 16);

    unpack_meta(meta, m, k, slot);
    reward = float_from_q(q);
}


// 24-bit integer payload → 0..16,777,215 representing reward * 1e4
inline uint32_t get_reward_q(const OPT& p) noexcept {
    return  (static_cast<uint32_t>(p.bytes[2])      ) |
           ((static_cast<uint32_t>(p.bytes[3]) << 8)) |
           ((static_cast<uint32_t>(p.bytes[4]) << 16));
}

// Float reward (scale = 1e-4). Absolute step ≈ 0.0001
inline float get_reward(const OPT& p) noexcept {
    return static_cast<float>(get_reward_q(p)) * 0.0001f;  // 1/10000
}


/*
table[j].reward represents opt(n, t, B)
table[j].mn is the server that user n is assigned to
table[j].kn is the accuracy tier that user n is assigned to.
table[j].slot is the number of time slots used by user n.

the tuple is
(n, t, C[1], R[1], C[2], R[2], ..., C[M], R[M]) ==> j
n=0, 1, 2, ..., N;
t=0, 1, 2, ..., T;
C[i]=0, 1, 2, ..., s[i].cpu;
R[i]=0, 1, 2, ..., s[i].ram;
i=1, 2, ..., M.

*/


#endif
