/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For strcasecmp
#include "apex_cpu.h"

#define MAX_COMMAND_LENGTH 100

void display_menu() {
    printf("\nAvailable commands:\n");
    printf("  SetMem <dfilename> - Initialize data memory\n");
    printf("  Initialize - Initialize simulator state\n");
    printf("  Simulate <n> - Simulate n cycles\n");
    printf("  Single_step - Advance simulation by one cycle\n");
    printf("  Display - Show pipeline, registers, and memory state\n");
    printf("  ShowMem <address> - Display content of specific memory location\n");
    printf("  Exit - Quit the simulator\n");
}

void set_mem(APEX_CPU *cpu, const char *dfilename) {
    FILE *fp = fopen(dfilename, "r");
    if (!fp) {
        printf("Error: Unable to open file %s\n", dfilename);
        return;
    }

    int address = 0;
    int value;
    while (fscanf(fp, "%d,", &value) == 1 && address < DATA_MEMORY_SIZE) {
        cpu->data_memory[address] = value;
        address++;
    }

    fclose(fp);
    printf("Data memory initialized from file %s\n", dfilename);
}

void initialize(APEX_CPU *cpu) {
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    cpu->clock = 0;
    cpu->insn_completed = 0;
    cpu->fetch.has_insn = TRUE;
    printf("Simulator state initialized\n");
}

void simulate(APEX_CPU *cpu, int num_cycles) {
    int i;
    for (i = 0; i < num_cycles; i++) {
        if (APEX_cpu_run_single_cycle(cpu)) {
            printf("Simulation stopped due to HALT instruction\n");
            break;
        }
    }
    printf("Simulated %d cycles\n", i);
}

void single_step(APEX_CPU *cpu) {
    if (APEX_cpu_run_single_cycle(cpu)) {
        printf("Simulation stopped due to HALT instruction\n");
    } else {
        printf("Simulated 1 cycle\n");
    }
}

void display(APEX_CPU *cpu) {
    print_reg_file(cpu);
    print_pipeline_state(cpu);
    print_memory(cpu, 0, 10);
}

void show_mem(APEX_CPU *cpu, int address) {
    if (address >= 0 && address < DATA_MEMORY_SIZE) {
        printf("Memory[%d] = %d\n", address, cpu->data_memory[address]);
    } else {
        printf("Error: Invalid memory address\n");
    }
}

int main(int argc, char const *argv[]) {
    APEX_CPU *cpu;
    char command[MAX_COMMAND_LENGTH];
    char *token;
    int num_cycles, address;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

    if (argc != 2) {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    cpu = APEX_cpu_init(argv[1]);
    if (!cpu) {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
    }

    while (1) {
        display_menu();
        printf("Enter command: ");
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        command[strcspn(command, "\n")] = 0;  // Remove newline

        token = strtok(command, " ");
        if (token == NULL) continue;

        if (strcasecmp(token, "SetMem") == 0) {
            token = strtok(NULL, " ");
            if (token) set_mem(cpu, token);
            else printf("Error: Missing filename for SetMem command\n");
        } else if (strcasecmp(token, "Initialize") == 0) {
            initialize(cpu);
        } else if (strcasecmp(token, "Simulate") == 0) {
            token = strtok(NULL, " ");
            if (token && sscanf(token, "%d", &num_cycles) == 1) {
                simulate(cpu, num_cycles);
            } else {
                printf("Error: Invalid number of cycles for Simulate command\n");
            }
        } else if (strcasecmp(token, "Single_step") == 0) {
            single_step(cpu);
        } else if (strcasecmp(token, "Display") == 0) {
            display(cpu);
        } else if (strcasecmp(token, "ShowMem") == 0) {
            token = strtok(NULL, " ");
            if (token && sscanf(token, "%d", &address) == 1) {
                show_mem(cpu, address);
            } else {
                printf("Error: Invalid address for ShowMem command\n");
            }
        } else if (strcasecmp(token, "Exit") == 0) {
            break;
        } else {
            printf("Error: Unknown command\n");
        }
    }

    APEX_cpu_stop(cpu);
    return 0;
}