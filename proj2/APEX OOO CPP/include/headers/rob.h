#ifndef _ROB_H_
#define _ROB_H_

#include "apex_cpu_types.h"
#include <stdint.h>

struct ROB_Entry {
    uint32_t pc;
    InstructionType type;
    uint32_t dest_arch_reg;
    uint32_t dest_phys_reg;
    uint32_t old_phys_reg;
    bool completed;
    uint32_t value;
    bool exception;
    bool mispredicted;
    uint32_t target_addr;
    uint32_t control_tag;

    ROB_Entry() {
        completed = false;
        exception = false;
        mispredicted = false;
    }
};

class ROB {
private:
    ROB_Entry entries[80];
    int head;
    int tail;
    int count;

public:
    ROB();
    
    int add_entry(uint32_t pc, InstructionType type, 
                 uint32_t dest_arch_reg, uint32_t dest_phys_reg,
                 uint32_t old_phys_reg, uint32_t control_tag);
    
    bool commit_entry();
    void write_result(int rob_idx, uint32_t value, bool mispredict);
    void rollback(int rob_idx);
    
    bool is_full();
    bool is_empty();
    ROB_Entry* get_entry(int index);
    int get_head() { return head; }
    int get_tail() { return tail; }
    int get_count() { return count; }
    void display_status();
};

#endif