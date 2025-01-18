// include/headers/lsq.h
#ifndef _LSQ_H_
#define _LSQ_H_

#include "apex_cpu_types.h"
#include <stdint.h>

struct LSQ_Entry {
    // Core Fields
    uint32_t address;           // Memory address
    uint32_t data;             // Data value/register value for store
    bool is_store;             // Load/Store indicator
    bool address_ready;        // Address calculation status
    uint32_t rob_index;        // Associated ROB entry
    
    // Register Dependencies
    uint32_t base_reg_tag;     // Base register physical tag
    uint32_t offset_reg_tag;   // Offset register physical tag
    uint32_t data_reg_tag;     // Data register tag (for stores)
    bool base_ready;           // Base register value available
    bool offset_ready;         // Offset register value available
    bool data_ready;           // Data value available (for stores)
    
    // Values
    uint32_t base_value;       // Base register value
    uint32_t offset_value;     // Offset value
    
    // Control
    uint32_t age;              // For ordering memory operations
    bool completed;            // Execution status
    
    LSQ_Entry() {
        address_ready = false;
        base_ready = false;
        offset_ready = false;
        data_ready = false;
        completed = false;
    }
};

class LSQ {
private:
    LSQ_Entry entries[6];      // 6-entry queue
    int head;                  // Oldest entry
    int tail;                  // Next free entry
    int count;                // Number of valid entries
    

public:
    LSQ();
    // Add these getters
    uint32_t get_address(int index) const {
        return (index >= 0 && index < 6) ? entries[index].address : 0;
    }
    
    bool is_store(int index) const {
        return (index >= 0 && index < 6) ? entries[index].is_store : false;
    }
    
    uint32_t get_data(int index) const {
        return (index >= 0 && index < 6) ? entries[index].data : 0;
    }
    
    // Core Functions
    int add_entry(bool is_store, uint32_t rob_idx);
    bool can_execute(int index);
    void set_address(int index, uint32_t addr);
    void set_data(int index, uint32_t data);
    void complete_entry(int index);
    void remove_entry();  // Called after commit
    int get_head() const { return head; }
    
    // Dependency Management
    void set_base_tag(int index, uint32_t tag);
    void set_offset_tag(int index, uint32_t tag);
    void set_data_tag(int index, uint32_t tag);
    void update_tag(uint32_t tag, uint32_t value);
    
    // Utility Functions
    bool is_full() { return count == 6; }
    bool is_empty() { return count == 0; }
    void display_status();
};

#endif