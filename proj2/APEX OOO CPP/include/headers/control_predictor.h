// include/headers/control_predictor.h
#ifndef _CONTROL_PREDICTOR_H_
#define _CONTROL_PREDICTOR_H_

#include "apex_cpu_types.h"
#include <stdint.h>

struct PredictorEntry {
    bool established;         // Entry validity
    PredictorType type;      // BRANCH/JALP/RET
    uint32_t target_addr;    // Predicted target
    bool last_outcome;       // For positive offset branches
    uint32_t pc;            // Instruction address
    int ras_index;          // RAS pointer for RET
};

struct ReturnStack {
    uint32_t addresses[4];   // 4-entry RAS
    int top;                // Stack pointer
};

class ControlPredictor {
private:
    PredictorEntry table[8];
    ReturnStack ras;
    int head;
    int count;

    int find_entry(uint32_t pc) const {
        for (int i = 0; i < 8; i++) {
            if (table[i].established && table[i].pc == pc) {
                return i;
            }
        }
        return -1;
    }

    int allocate_entry() {
        int index = head;
        head = (head + 1) % 8;
        if (count < 8) count++;
        return index;
    }

public:
    ControlPredictor();
    
    // Core prediction functions
    bool lookup_prediction(uint32_t pc, PredictorType type, int32_t offset, uint32_t& target);
    void establish_entry(uint32_t pc, PredictorType type, int32_t offset);
    void update_prediction(uint32_t pc, bool actual_outcome, uint32_t target);
    
    // Return stack management
    void push_return_address(uint32_t addr);
    uint32_t pop_return_address();
    
    // Query functions
    bool was_predicted_taken(uint32_t pc) const;
    void display_status() const;
};

#endif