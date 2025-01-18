#ifndef _MEMORY_FU_H_
#define _MEMORY_FU_H_

#include "apex_cpu_types.h"
#include "lsq.h"
#include <stdint.h>

// Structure to represent each stage of the 3-stage Memory FU
struct MemStage {
    bool busy;
    int lsq_index;    // Index of LSQ entry being processed
    uint32_t address; // Memory address being accessed
    uint32_t data;    // Data being read/written
    bool is_store;    // Type of operation
};

class MemoryFU {
private:
    LSQ& lsq;                 // Reference to LSQ
    MemStage stages[3];       // 3 pipeline stages
    uint32_t memory[4096];    // Simple memory array for simulation

    // Internal function to move operations through stages
    void advance_stages();
    
public:
    MemoryFU(LSQ& lsq_ref);
    
    // Core Functions
    bool can_accept();  // Check if FU can accept new operation
    bool issue(int lsq_index);  // Try to issue LSQ entry to FU
    void execute();  // Execute one cycle
    
    // Memory Access Functions
    uint32_t read_memory(uint32_t address);
    void write_memory(uint32_t address, uint32_t data);
    
    // Debug/Display
    void display_status();
};

#endif