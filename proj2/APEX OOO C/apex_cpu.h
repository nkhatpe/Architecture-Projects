#ifndef APEX_CPU_H
#define APEX_CPU_H

// Include all necessary headers and definitions
#include <stdint.h>
#include "apex_macros.h"

#define ROB_SIZE 80
#define PHYS_REG_COUNT 64

/* Reorder Buffer (ROB) Entry */
typedef struct ROB_Entry {
    int pc;               // Instruction address
    int dest_arch_reg;    // Destination architectural register
    int dest_phys_reg;    // Destination physical register
    int value;            // Execution result
    int completed;        // Status: 0 (not completed), 1 (completed)
    int mispredicted;     // Misprediction flag
    int target_addr;      // Target address for branches
} ROB_Entry;

/* Reorder Buffer (ROB) */
typedef struct ROB {
    ROB_Entry entries[ROB_SIZE];  // Circular buffer for ROB entries
    int head;                     // Head pointer for committing instructions
    int tail;                     // Tail pointer for adding instructions
    int count;                    // Number of valid entries in ROB
} ROB;

/* Rename Table */
typedef struct RenameTable {
    int arch_to_phys[REG_FILE_SIZE];   // Mapping from architectural to physical registers
    int phys_to_arch[PHYS_REG_COUNT]; // Mapping from physical to architectural registers
    int free_phys_regs[PHYS_REG_COUNT]; // Free physical register list
    int free_count;                    // Number of free physical registers
} RenameTable;

/* Branch Predictor */
typedef struct BranchPredictor {
    int history[8]; // Simplified 2-bit predictor for 8 branches
    int head;       // Circular head pointer
} BranchPredictor;

/* Format of an APEX instruction */
typedef struct APEX_Instruction {
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage {
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rd;
    int imm;
    int rs1_value;
    int rs2_value;
    int result_buffer;
    int memory_address;
    int has_insn;
} CPU_Stage;

/* Model of APEX CPU */
typedef struct APEX_CPU {
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
    int regs[REG_FILE_SIZE];       /* Integer register file */
    int code_memory_size;          /* Number of instructions in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int fetch_from_next_cycle;

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage writeback;

    /* Out-of-Order Execution Components */
    ROB rob;                        /* Reorder Buffer */
    RenameTable rename_table;       /* Rename Table */
    BranchPredictor branch_predictor; /* Branch Predictor */
    int is_halted; /* Flag to stop simulation on HALT */

} APEX_CPU;

/* Function Declarations */
APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);

/* New Function Declarations */
void init_ROB(ROB *rob);
int add_ROB_entry(ROB *rob, int pc, int dest_arch_reg, int dest_phys_reg);
void commit_ROB_entry(ROB *rob, int *regs, int *zero_flag);

void init_RenameTable(RenameTable *rename_table);
int allocate_phys_reg(RenameTable *rename_table);
void free_phys_reg(RenameTable *rename_table, int phys_reg);

void init_BranchPredictor(BranchPredictor *bp);
int predict_branch(BranchPredictor *bp);
void update_branch(BranchPredictor *bp, int taken);

void Fetch(APEX_CPU *cpu);
void Decode(APEX_CPU *cpu);
void Dispatch(APEX_CPU *cpu);
void Execute(APEX_CPU *cpu);
void Commit(APEX_CPU *cpu);
void SetMem(int *memory, int size);



#endif
