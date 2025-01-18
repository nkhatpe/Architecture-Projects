// src/int_fu.cpp
#include "int_fu.h"
#include <stdio.h>

IntegerFU::IntegerFU(ControlPredictor& pred_ref) 
    : predictor(pred_ref), busy(false), executing(false) {
}

bool IntegerFU::can_accept() {
    return !busy;
}

bool IntegerFU::issue(uint32_t pc, InstructionType op, uint32_t s1, uint32_t s2, uint32_t rob_idx) {
    if (busy) {
        return false;
    }

    current_pc = pc;
    src1_value = s1;
    src2_value = s2;
    op_type = op;
    rob_index = rob_idx;
    busy = true;
    executing = false;

    printf("IntFU: Issued %d operation, PC=0x%x, src1=0x%x, src2=0x%x\n",
           op_type, current_pc, src1_value, src2_value);
    return true;
}

uint8_t IntegerFU::calculate_flags(uint32_t result) {
    uint8_t flags = 0;
    if (result == 0) flags |= 0x4;  // Zero flag
    else if ((int32_t)result < 0) flags |= 0x2;  // Negative flag
    else flags |= 0x1;  // Positive flag
    return flags;
}

uint32_t IntegerFU::calculate_branch_target(uint32_t pc, int32_t offset) {
    return pc + offset;
}

IntFUResult IntegerFU::execute() {
    IntFUResult result = {0};
    
    if (!busy || executing) {
        return result;
    }

    executing = true;
    printf("IntFU: Executing operation type %d\n", op_type);

    // Declare variables outside switch to avoid crossing initialization
    uint32_t target;
    bool taken;

    switch(op_type) {
        case INT_ADD:
            result.value = src1_value + src2_value;
            result.cc_modified = true;
            result.cc_flags = calculate_flags(result.value);
            break;

        case INT_SUB:
            result.value = src1_value - src2_value;
            result.cc_modified = true;
            result.cc_flags = calculate_flags(result.value);
            break;

        case BRANCH: {
            target = calculate_branch_target(current_pc, (int32_t)src2_value);
            taken = false;
            
            // Check branch condition based on flags
            switch(src1_value & 0x7) {  // Check only lower 3 bits for Z,N,P flags
                case 0x4: taken = true;  // BZ and Zero flag set
                    break;
                case 0x2: taken = true;  // BN and Negative flag set
                    break;
                case 0x1: taken = true;  // BP and Positive flag set
                    break;
            }

            result.mispredicted = (taken != predictor.was_predicted_taken(current_pc));
            result.target = taken ? target : current_pc + 4;
            break;
        }

        case JALP: {
            result.target = calculate_branch_target(current_pc, (int32_t)src2_value);
            result.value = current_pc + 4;  // Return address
            predictor.push_return_address(result.value);
            break;
        }

        case RET: {
            result.target = src1_value;  // Return address from register
            break;
        }

        // Handle other instruction types...
        case INT:
        case MUL:
        case LOAD:
        case STORE:
        case CMP:
        case CML:
        case ADD:
            printf("IntFU: Unsupported operation type %d\n", op_type);
            break;
    }

    busy = false;
    executing = false;
    printf("IntFU: Completed operation, result=0x%x\n", result.value);
    return result;
}

void IntegerFU::display_status() {
    printf("\nInteger FU Status:\n");
    printf("Busy: %s\n", busy ? "YES" : "NO");
    if (busy) {
        printf("Current Operation: Type=%d, PC=0x%x\n", op_type, current_pc);
        printf("Source Values: 0x%x, 0x%x\n", src1_value, src2_value);
        printf("ROB Index: %d\n", rob_index);
    }
}