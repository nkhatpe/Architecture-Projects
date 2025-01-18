#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include <stdint.h>
#include "rob.h"
#include "register_manager.h"
#include "control_predictor.h"
#include "lsq.h"
#include "memory_fu.h" 
#include "int_fu.h"

class APEX_CPU {
private:
    int cycle;
    bool halt;
    ROB rob;
    RegisterManager reg_mgr;
    ControlPredictor predictor;
    LSQ lsq;
    MemoryFU mem_fu;
    IntegerFU int_fu;
    
public:
    APEX_CPU();
    ~APEX_CPU();
    
    void initialize();
    void run_cpu();
    void single_step();
    void show_state();
};

#endif