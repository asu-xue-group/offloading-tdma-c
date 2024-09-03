#ifndef TDMA_KNAPSACK_OUTPUT_HPP
#define TDMA_KNAPSACK_OUTPUT_HPP

#include <vector>
#include <array>
#include <string>

void print_to_file(const std::string &filename, const std::vector<std::array<int, 4>> &solution, int optimal,
                   double time, double mem_used) {
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file %s\n", filename.c_str());
        exit(0);
    }

    fprintf(fp, "T=%d, lambda=%d\n", T, lambda);
    fprintf(fp, "Optimal value: %d\n", optimal);
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
    fprintf(fp, "Memory used: %.2f MB\n", mem_used);

    fclose(fp);
}


#endif //TDMA_KNAPSACK_OUTPUT_HPP
