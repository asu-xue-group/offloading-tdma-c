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


struct OPT {
//    unsigned char mn;         // this user is assigned to server m, mn=0 means we skip this user.
//    unsigned char kn;         // this user is assigned to accuracy k
    unsigned char solution;   // multiplexed server and accuracy
    unsigned char slot;      // the number of time slots assigned to this user
    float reward;     // the corresponding optimal value
};

struct OPT_PATH {
    int required_T = -1;
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
