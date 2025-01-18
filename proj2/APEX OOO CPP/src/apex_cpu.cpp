#include <stdio.h>
#include "apex_cpu.h"

APEX_CPU::APEX_CPU()
    : mem_fu(lsq)  // Initialize mem_fu with reference to lsq
    , int_fu(predictor)
{
    cycle = 0;
    halt = false;
}

APEX_CPU::~APEX_CPU() {
    // Cleanup
}

void APEX_CPU::initialize() {
    // Initialize components
}

void APEX_CPU::run_cpu() {
    while (!halt) {
        single_step();
    }
}

void APEX_CPU::single_step() {
    // Will implement pipeline stages here
    cycle++;
}

void APEX_CPU::show_state() {
    printf("===== CPU State =====\n");
    printf("Cycle: %d\n", cycle);
    // Will add more state display
}