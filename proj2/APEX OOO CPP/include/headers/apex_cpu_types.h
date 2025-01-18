#ifndef _APEX_CPU_TYPES_H_
#define _APEX_CPU_TYPES_H_

#include <stdint.h>

// Instruction Types
enum InstructionType {
    INT,
    INT_ADD,
    INT_SUB,
    MUL,
    LOAD,
    STORE,
    BRANCH,
    JALP,
    RET,
    CMP,
    CML,
    ADD
};

// Predictor Types
enum PredictorType {
    PRED_BRANCH,
    PRED_JALP,
    PRED_RET
};

#endif