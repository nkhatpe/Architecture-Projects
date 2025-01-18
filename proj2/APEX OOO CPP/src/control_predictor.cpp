#include "control_predictor.h"
#include <stdio.h>
#include <cstdlib> 

ControlPredictor::ControlPredictor() {
    head = 0;
    count = 0;
    
    // Initialize predictor entries
    for (int i = 0; i < 8; i++) {
        table[i].established = false;
    }
}

bool ControlPredictor::lookup_prediction(uint32_t pc, PredictorType type, 
                                       int32_t offset, uint32_t& target) {
    const int index = find_entry(pc);
    
    if (index != -1) {  // Hit
        if (type == PRED_BRANCH) {  // Change BRANCH to PRED_BRANCH
            if (offset < 0) {
                target = table[index].target_addr;
                return true;  // Always predict taken for backward branches
            } else {
                target = table[index].target_addr;
                return table[index].last_outcome;  // Use last outcome for forward branches
            }
        } else if (type == PRED_JALP) {  // Change JALP to PRED_JALP
            target = table[index].target_addr;
            return true;  // Always predict taken
        } else if (type == PRED_RET) {  // Change RET to PRED_RET
            target = pop_return_address();
            return true;  // Always predict taken
        }
    }
    
    // Miss case: compute default target
    if (type == PRED_BRANCH) {
        target = pc + offset;  // Calculate branch target
    } else if (type == PRED_RET && ras.top >= 0) {
        target = ras.addresses[ras.top];  // Use top of RAS
    } else {
        target = pc + 4;  // Default sequential
    }
    
    return false;
}

void ControlPredictor::establish_entry(uint32_t pc, PredictorType type, int32_t offset) {
    int index = allocate_entry();
    table[index].established = true;
    table[index].pc = pc;
    table[index].type = type;
    table[index].last_outcome = (offset < 0);  // Default prediction for branches
    
    // Fix target calculation based on type
    if (type == PRED_BRANCH) {
        table[index].target_addr = pc + offset;
    } else if (type == PRED_JALP) {
        table[index].target_addr = pc + offset;  // Use provided offset for JALP
    } else {
        table[index].target_addr = pc + 4;  // Default for RET
    }
}

bool ControlPredictor::was_predicted_taken(uint32_t pc) const {
    int index = find_entry(pc);
    if (index != -1) {
        if (table[index].type == PRED_BRANCH) {
            return table[index].last_outcome;
        }
    }
    // Default prediction for branches not in predictor
    return false;
}

void ControlPredictor::update_prediction(uint32_t pc, bool actual_outcome, uint32_t target) {
    int index = find_entry(pc);
    if (index != -1) {
        if (target != 0) {  // Only update if valid target provided
            table[index].target_addr = target;
        }
        table[index].last_outcome = actual_outcome;
    }
}

void ControlPredictor::push_return_address(uint32_t addr) {
    if (ras.top < 3) {  // Ensure we don't exceed array bounds
        ras.top++;
        ras.addresses[ras.top] = addr;
        printf("RAS Push: Index=%d, Address=0x%x\n", ras.top, addr);
    }
}

uint32_t ControlPredictor::pop_return_address() {  // Changed return type from void to uint32_t
    uint32_t addr = 0;
    if (ras.top >= 0) {
        addr = ras.addresses[ras.top];
        printf("RAS Pop: Index=%d, Address=0x%x\n", ras.top, addr);
        ras.addresses[ras.top] = 0;  // Clear the entry
        ras.top--;
    }
    return addr;
}

void ControlPredictor::display_status() const {
    printf("\nControl Predictor Status:\n");
    printf("Valid Entries: %d\n", count);
    printf("Next replacement index: %d\n", head);
    
    printf("\nPredictor Table:\n");
    for (int i = 0; i < 8; i++) {
        if (table[i].established) {
            printf("Entry %d: PC=0x%x Type=%d LastOutcome=%d Target=0x%x\n",
                   i, table[i].pc, table[i].type, table[i].last_outcome, 
                   table[i].target_addr);
        }
    }
    
    printf("\nReturn Address Stack:\n");
    printf("Top: %d\n", ras.top);
    for (int i = ras.top; i >= 0; i--) {
        printf("RAS[%d] = 0x%x\n", i, ras.addresses[i]);
    }
}