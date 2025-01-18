#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apex_macros.h"
#include "apex_cpu.h"

// Pipeline Stages
void Fetch(APEX_CPU *cpu);
void Decode(APEX_CPU *cpu);
void Dispatch(APEX_CPU *cpu);
void Execute(APEX_CPU *cpu);
void Commit(APEX_CPU *cpu);

// Utility to Display Memory Contents
void DisplayMem(int *memory, int size) {
    printf("Non-zero Memory Contents:\n");
    for (int i = 0; i < size; i++) {
        if (memory[i] != 0) { // Display only non-zero memory locations
            printf("Mem[%d] = %d\n", i, memory[i]);
        }
    }
}

// Utility to Initialize Memory (SetMem)
void SetMem(int *memory, int size) {
    printf("Enter %d memory values separated by spaces (or press Enter to use default values):\n", size);
    
    char input[256];
    if (fgets(input, sizeof(input), stdin) != NULL && strlen(input) > 1) {
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < size) {
            memory[i] = atoi(token);
            token = strtok(NULL, " ");
            i++;
        }
        if (i < size) {
            for (; i < size; i++) {
                memory[i] = 0;
            }
        }
    } else {
        int default_values[] = {1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14};
        for (int i = 0; i < size && i < 16; i++) {
            memory[i] = default_values[i];
        }
        if (size > 16) {
            for (int i = 16; i < size; i++) {
                memory[i] = 0;
            }
        }
        printf("Memory initialized with default values.\n");
    }

    // Display non-zero memory values after initialization
    DisplayMem(memory, size);
}

void Fetch(APEX_CPU *cpu) {
    // Fetch logic: Fetch instructions using Predictor
}

void Decode(APEX_CPU *cpu) {
    // Decode logic: Decode instructions and rename registers
}

void Dispatch(APEX_CPU *cpu) {
    int dest_phys_reg = allocate_phys_reg(&cpu->rename_table);
    if (dest_phys_reg == -1) {
        return;
    }
    add_ROB_entry(&cpu->rob, cpu->decode.pc, cpu->decode.rd, dest_phys_reg);
}

void Execute(APEX_CPU *cpu) {
    // Execute logic: Process instructions in functional units
}

void Commit(APEX_CPU *cpu) {
    commit_ROB_entry(&cpu->rob, cpu->regs, &cpu->zero_flag);
}

void run_simulator(APEX_CPU *cpu, int *memory) {
    int memory_size = DATA_MEMORY_SIZE;

    // Initialize memory dynamically
    SetMem(memory, memory_size);

    // Simulation Loop
    for (int cycle = 0; cycle < 100; cycle++) {
        if (cpu->is_halted) { // Stop simulation if HALT is encountered
            printf("Simulation halted at cycle %d.\n", cycle);
            break;
        }

        Commit(cpu);
        Execute(cpu);
        Dispatch(cpu);
        Decode(cpu);
        Fetch(cpu);
    }

    // Display non-zero memory contents at the end of the simulation
    printf("\nFinal Non-zero Memory State:\n");
    DisplayMem(memory, memory_size);
}
