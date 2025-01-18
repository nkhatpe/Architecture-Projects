#include "memory_fu.h"
#include <stdio.h>

MemoryFU::MemoryFU(LSQ& lsq_ref) : lsq(lsq_ref) {
    // Initialize stages
    for(int i = 0; i < 3; i++) {
        stages[i].busy = false;
        stages[i].lsq_index = -1;
    }
    
    // Initialize memory (for simulation)
    for(uint32_t i = 0; i < 4096; i++) {
        memory[i] = 0;
    }
}

bool MemoryFU::can_accept() {
    return !stages[0].busy;
}

bool MemoryFU::issue(int lsq_index) {
    if(!can_accept()) {
        return false;
    }
    
    // Get LSQ entry information
    uint32_t address = lsq.get_address(lsq_index);  // Need to add this getter to LSQ
    bool is_store = lsq.is_store(lsq_index);        // Need to add this getter to LSQ
    uint32_t data = is_store ? lsq.get_data(lsq_index) : 0;  // For stores
    
    // Set up first stage
    stages[0].busy = true;
    stages[0].lsq_index = lsq_index;
    stages[0].address = address;
    stages[0].is_store = is_store;
    stages[0].data = data;
    
    printf("MemFU: Issued LSQ entry %d to stage 0 (%s Addr:0x%x)\n", 
           lsq_index, is_store ? "STORE" : "LOAD", address);
    return true;
}

void MemoryFU::advance_stages() {
    // Move from stage 2 to completion
    if(stages[2].busy) {
        if(stages[2].is_store) {
            write_memory(stages[2].address, stages[2].data);
        }
        stages[2].busy = false;
        printf("MemFU: Completed %s operation at address 0x%x\n", 
               stages[2].is_store ? "STORE" : "LOAD",
               stages[2].address);
    }
    
    // Move from stage 1 to 2
    if(stages[1].busy) {
        stages[2] = stages[1];
        stages[1].busy = false;
        printf("MemFU: Advanced stage 1 to 2\n");
    }
    
    // Move from stage 0 to 1
    if(stages[0].busy) {
        stages[1] = stages[0];
        stages[0].busy = false;
        
        // If LOAD, perform the read in stage 1
        if(!stages[1].is_store) {
            stages[1].data = read_memory(stages[1].address);
        }
        printf("MemFU: Advanced stage 0 to 1\n");
    }
}

void MemoryFU::execute() {
    printf("\nMemFU Execute Cycle:\n");
    
    // First advance existing operations
    advance_stages();
    
    // Process each stage
    for(int i = 0; i < 3; i++) {
        if(stages[i].busy) {
            printf("Stage %d: Processing LSQ entry %d\n", 
                   i, stages[i].lsq_index);
        }
    }
}

uint32_t MemoryFU::read_memory(uint32_t address) {
    if(address < 4096) {
        printf("MemFU: Reading 0x%x from address 0x%x\n", 
               memory[address], address);
        return memory[address];
    }
    return 0;
}

void MemoryFU::write_memory(uint32_t address, uint32_t data) {
    if(address < 4096) {
        printf("MemFU: Writing 0x%x to address 0x%x\n", 
               data, address);
        memory[address] = data;
    }
}

void MemoryFU::display_status() {
    printf("\nMemory FU Status:\n");
    for(int i = 0; i < 3; i++) {
        printf("Stage %d: %s", i, stages[i].busy ? "BUSY" : "FREE");
        if(stages[i].busy) {
            printf(" - LSQ:%d %s Addr:0x%x", 
                   stages[i].lsq_index,
                   stages[i].is_store ? "STORE" : "LOAD",
                   stages[i].address);
            if(stages[i].is_store || i > 0) {  // Show data for stores or after loads
                printf(" Data:0x%x", stages[i].data);
            }
        }
        printf("\n");
    }
}