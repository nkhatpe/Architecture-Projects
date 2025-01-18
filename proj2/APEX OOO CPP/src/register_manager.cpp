#include "register_manager.h"
#include <stdio.h>

RegisterManager::RegisterManager() {
    // Initialize frontend and backend RATs
    for (int i = 0; i < 32; i++) {
        frontend_rat[i].phys_reg = i;
        frontend_rat[i].valid = true;
        backend_rat[i].phys_reg = i;
        backend_rat[i].valid = true;
    }

    frontend_cc_rat[0].phys_reg = 0;
    frontend_cc_rat[0].valid = true;
    backend_cc_rat[0].phys_reg = 0;
    backend_cc_rat[0].valid = true;

    // Initialize free lists
    for (uint32_t i = 32; i < 60; i++) {
        free_list_uprf.push(i);
    }
    for (uint32_t i = 1; i < 10; i++) {
        free_list_ucrf.push(i);
    }

    // Initialize valid bits
    for (int i = 0; i < 60; i++) {
        uprf_valid[i] = true;
    }
    for (int i = 0; i < 10; i++) {
        ucrf_valid[i] = true;
    }
}

RegisterManager::~RegisterManager() {
    // Cleanup if needed
}

uint32_t RegisterManager::allocate_physical_register() {
    if (free_list_uprf.empty()) {
        return -1;  // No free registers
    }

    uint32_t reg = free_list_uprf.front();
    free_list_uprf.pop();
    uprf_valid[reg] = false;  // Mark as allocated
    return reg;
}

void RegisterManager::update_frontend_table(uint32_t arch_reg, uint32_t phys_reg) {
    if (arch_reg < 32) {
        frontend_rat[arch_reg].phys_reg = phys_reg;
        frontend_rat[arch_reg].valid = true;
    }
}

void RegisterManager::update_backend_table(uint32_t arch_reg, uint32_t phys_reg) {
    if (arch_reg < 32) {
        // Free the old physical register
        uint32_t old_phys_reg = backend_rat[arch_reg].phys_reg;
        free_physical_register(old_phys_reg);

        // Update backend table
        backend_rat[arch_reg].phys_reg = phys_reg;
        backend_rat[arch_reg].valid = true;
    }
}

void RegisterManager::free_physical_register(uint32_t phys_reg) {
    if (phys_reg >= 32 && phys_reg < 60) {
        free_list_uprf.push(phys_reg);
        uprf_valid[phys_reg] = true;
    }
}

int RegisterManager::create_checkpoint(uint32_t control_tag) {
    Checkpoint cp;
    
    // Save rename table state
    for (int i = 0; i < 32; i++) {
        cp.arch_to_phys[i] = frontend_rat[i];
    }
    cp.cc_to_phys[0] = frontend_cc_rat[0];

    // Save free lists
    cp.free_list_uprf = free_list_uprf;
    cp.free_list_ucrf = free_list_ucrf;

    // Save valid bits
    for (int i = 0; i < 60; i++) {
        cp.valid_bits[i] = uprf_valid[i];
    }
    for (int i = 0; i < 10; i++) {
        cp.cc_valid_bits[i] = ucrf_valid[i];
    }

    cp.control_tag = control_tag;

    checkpoints.push_back(cp);
    return checkpoints.size() - 1;
}

// Add this to src/register_manager.cpp
void RegisterManager::restore_checkpoint(int checkpoint_id) {
    if (checkpoint_id < 0 || checkpoint_id >= (int)checkpoints.size()) {
        printf("Invalid checkpoint ID\n");
        return;
    }

    Checkpoint& cp = checkpoints[checkpoint_id];

    // Restore rename table state
    for (int i = 0; i < 32; i++) {
        frontend_rat[i] = cp.arch_to_phys[i];
    }
    frontend_cc_rat[0] = cp.cc_to_phys[0];

    // Restore free lists
    free_list_uprf = cp.free_list_uprf;
    free_list_ucrf = cp.free_list_ucrf;

    // Restore valid bits
    for (int i = 0; i < 60; i++) {
        uprf_valid[i] = cp.valid_bits[i];
    }
    for (int i = 0; i < 10; i++) {
        ucrf_valid[i] = cp.cc_valid_bits[i];
    }

    printf("Restored checkpoint %d\n", checkpoint_id);
}

void RegisterManager::display_status() {
    printf("\nRegister Manager Status:\n");
    printf("Free Physical Registers: %lu\n", free_list_uprf.size());
    printf("Free CC Registers: %lu\n", free_list_ucrf.size());
    
    printf("\nFrontend RAT:\n");
    for (int i = 0; i < 32; i++) {
        if (frontend_rat[i].valid) {
            printf("R%d -> P%d\n", i, frontend_rat[i].phys_reg);
        }
    }
    
    printf("\nBackend RAT:\n");
    for (int i = 0; i < 32; i++) {
        if (backend_rat[i].valid) {
            printf("R%d -> P%d\n", i, backend_rat[i].phys_reg);
        }
    }
}