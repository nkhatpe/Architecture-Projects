/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */

#include <stdio.h>
#include <stdlib.h>

#include "apex_cpu.h"

// Declare the run_simulator function from out_of_order_simulator.c
void run_simulator(APEX_CPU *cpu, int *memory);

int main(int argc, char const *argv[])
{
    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

    if (argc != 2)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    // Initialize CPU
    APEX_CPU *cpu = APEX_cpu_init(argv[1]);
    if (!cpu)
    {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
    }

    // Initialize memory
    int memory[4096] = {0};

    // Run the Out-of-Order Simulator
    run_simulator(cpu, memory);

    // Start and stop the original APEX CPU simulation
    APEX_cpu_run(cpu);
    APEX_cpu_stop(cpu); // Ensure this handles freeing

    return 0; // Remove duplicate free(cpu) call
}
