#ifndef _structs_h
#define _structs_h 1


#include <vector>


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
    TIER tier[3];    // task requirement and reward: tier[1] and tier[2], wasting tier[0]...
};


/* ┌─ meta (32 b) ───────────────────────────────────────────────┐
   │  3             2             1             0                │
   │3 1             4             6             0                │
   ├┴──┬─┬──────┬──────┬──────────┬──────────┬───────────────────┤
   │RSV│ num_frame     │   slot   │  k  │  m │  (bit index)      │
   └───┴─┴──────┴──────┴──────────┴──────────┴───────────────────┘
      24-31    12-17    6-11       4-5    0-3      = 18 used bits
      ^ 14 spare bits for flags / larger ranges ↓                 */

#pragma pack(push,1)               // eliminate alignment padding
struct OPT {
    uint32_t meta;     // packed control bits (see above)
    uint16_t reward_q; // reward ×100, fixed-point (0-655.35)
};
#pragma pack(pop)

static_assert(sizeof(OPT)==6, "OPT6 must be 6 bytes");

inline uint32_t pack_meta(uint8_t m, uint8_t k,
                          uint8_t slot, uint8_t frame)
{
    return (m & 0x0F)           |               // bits 0-3
           ((k    & 0x03) << 4) |               // bits 4-5
           ((slot & 0x3F) << 6) |               // bits 6-11
           ((frame& 0x3F) << 12);               // bits 12-17
    /* bits 18-31 remain 0 (reserved) */
}

inline void unpack_meta(uint32_t meta,
                        uint8_t& m, uint8_t& k,
                        uint8_t& slot, uint8_t& frame)
{
    m     =  meta        & 0x0F;
    k     = (meta >> 4 ) & 0x03;
    slot  = (meta >> 6 ) & 0x3F;
    frame = (meta >> 12) & 0x3F;
}

inline uint16_t qnt_reward(float r){ return std::lround(r * 100.0f); }
inline float    get_reward(uint16_t q){ return q * 0.01f; }


struct OPT_PATH {
    int required_T = -1;
    int X_n = -1;
    std::vector<std::string> path;
    std::vector<int> timeslots;
};

/*
table[j].reward represents opt(n, t, B)
table[j].mn is the server that user n is assigned to
table[j].kn is the accuracy tier that user n is assigned to.
table[j].slot is the number of time slots used by user n.

the tupple is
(n, t, C[1], R[1], C[2], R[2], ..., C[M], R[M]) ==> j
n=0, 1, 2, ..., N;
t=0, 1, 2, ..., T;
C[i]=0, 1, 2, ..., s[i].cpu;
R[i]=0, 1, 2, ..., s[i].ram;
i=1, 2, ..., M.

*/


#endif
