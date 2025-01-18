// include/headers/int_fu.h
#ifndef _INT_FU_H_
#define _INT_FU_H_

#include "apex_cpu_types.h"
#include "control_predictor.h"
#include <stdint.h>

struct IntFUResult {
    uint32_t value;
    bool cc_modified;
    uint8_t cc_flags;    // Zero, Negative, Positive flags
    bool mispredicted;   // For branch instructions
    uint32_t target;     // Target address for control instructions
};

class IntegerFU {
private:
    ControlPredictor& predictor;
    bool busy;
    uint32_t current_pc;
    uint32_t src1_value;
    uint32_t src2_value;
    InstructionType op_type;
    uint32_t rob_index;
    bool executing;

    // Internal helper functions
    uint8_t calculate_flags(uint32_t result);
    uint32_t calculate_branch_target(uint32_t pc, int32_t offset);

public:
    IntegerFU(ControlPredictor& pred_ref);

    // Core Functions
    bool can_accept();
    bool issue(uint32_t pc, InstructionType op, uint32_t s1, uint32_t s2, uint32_t rob_idx);
    IntFUResult execute();
    
    // Status Functions
    bool is_busy() const { return busy; }
    void clear() { busy = false; executing = false; }
    
    // Debug Support
    void display_status();
};

#endif