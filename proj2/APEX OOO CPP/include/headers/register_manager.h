#ifndef _REGISTER_MANAGER_H_
#define _REGISTER_MANAGER_H_

#include "apex_cpu_types.h"
#include <queue>
#include <vector>  // Add this for std::vector
#include <stdint.h>
#include <stdio.h>

struct RenameTableEntry {
    uint32_t phys_reg;    // Physical register number
    bool valid;           // Valid bit
};

struct Checkpoint {
    RenameTableEntry arch_to_phys[32];  // Frontend rename table state
    RenameTableEntry cc_to_phys[1];     // CC flag mapping state
    std::queue<uint32_t> free_list_uprf;  // Free physical register list state
    std::queue<uint32_t> free_list_ucrf;  // Free CC register list state
    bool valid_bits[60];                  // UPRF valid bits
    bool cc_valid_bits[10];               // UCRF valid bits
    uint32_t control_tag;                 // Tag for this checkpoint
};

class RegisterManager {
private:
    // Rename Tables
    RenameTableEntry frontend_rat[32];  // Frontend RAT for regular registers
    RenameTableEntry backend_rat[32];   // Backend RAT for regular registers
    RenameTableEntry frontend_cc_rat[1]; // Frontend RAT for CC
    RenameTableEntry backend_cc_rat[1];  // Backend RAT for CC

    // Free Lists
    std::queue<uint32_t> free_list_uprf;  // Free list for physical registers
    std::queue<uint32_t> free_list_ucrf;  // Free list for CC registers

    // Valid Bits
    bool uprf_valid[60];   // Valid bits for physical registers
    bool ucrf_valid[10];   // Valid bits for CC registers

    // Checkpoint Management
    std::vector<Checkpoint> checkpoints;

public:
    RegisterManager();
    ~RegisterManager();

    // Core Functions
    uint32_t allocate_physical_register();
    uint32_t allocate_cc_register();
    void update_frontend_table(uint32_t arch_reg, uint32_t phys_reg);
    void update_frontend_cc(uint32_t phys_reg);
    void update_backend_table(uint32_t arch_reg, uint32_t phys_reg);
    void update_backend_cc(uint32_t phys_reg);
    void free_physical_register(uint32_t phys_reg);
    void free_cc_register(uint32_t phys_reg);

    // Checkpoint Functions
    int create_checkpoint(uint32_t control_tag);
    void restore_checkpoint(int checkpoint_id);
    void free_checkpoint(int checkpoint_id);

    // Utility Functions
    bool is_register_available();
    bool is_cc_available();
    uint32_t get_physical_register(uint32_t arch_reg);
    uint32_t get_cc_register();
    void display_status();

    // Testing Functions (optional)
    void run_tests();
};

#endif