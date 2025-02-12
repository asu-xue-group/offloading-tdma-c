#ifndef _structs_h
#define _structs_h 1


#include <vector>


struct SERVER {
    int index;                // index of the server, starting from 1...
    float x;                    // x coordinate of the server
    float y;                    // y coordinate of the server
    int cpu;                  // number of CPU cores
    int ram;                  // size of memory
};


struct TIER {
    int tier;       // tier in {1, 2}. This may not be needed, but we will keep it.
    int cpu;        // cpu requirement
    int ram;        // ram requirement
    float time;       // execution time
    int reward;     // corresponding reward
};

struct USER {
    int index;      // index of the user, starting from 1...
    float x;          // x coordinate of the user
    float y;          // y coordinate of the user
    float ddl;        // deadline
    float data;       // data size, in MB
    TIER tier[3];    // task requirement and reward: tier[1] and tier[2], wasting tier[0]...
};


struct OPT {
//    int mn;         // this user is assigned to server m, mn=0 means we skip this user.
//    int kn;         // this user is assigned to accuracy k
//    int slot;       // the number of time slots assigned to this user.
    unsigned short solution; // the "multiplexed" solution
    unsigned short reward;     // the corresponding optimal value
};

struct TIMING {
    int relay;      // the relay that this user is assigned to
    int T;         // the total timeslot this user-server pair is assigned
    int ur_time;    // the # timeslot it takes to transmit data from user to relay
    int rs_time;    // the # timeslot it takes to transmit data from relay to server
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
