/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_LTR:

        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
        
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            printf("%s,R%d, R%d, #%d ", stage->opcode_str, stage->rd, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }
        
        case OPCODE_CMP:
        {
            printf("%s, R%d, R%d ", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_JUMP:
        {
            printf("%s, R%d, #%d ", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_HALT:
        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}

void init_ROB(ROB *rob) {
    rob->head = 0;
    rob->tail = 0;
    rob->count = 0;
    for (int i = 0; i < ROB_SIZE; i++) {
        rob->entries[i].completed = 0;
        rob->entries[i].mispredicted = 0;
    }
}

int add_ROB_entry(ROB *rob, int pc, int dest_arch_reg, int dest_phys_reg) {
    if (rob->count == ROB_SIZE) {
        return -1;  // ROB is full
    }

    ROB_Entry *entry = &rob->entries[rob->tail];
    entry->pc = pc;
    entry->dest_arch_reg = dest_arch_reg;
    entry->dest_phys_reg = dest_phys_reg;
    entry->completed = 0;
    entry->mispredicted = 0;

    rob->tail = (rob->tail + 1) % ROB_SIZE;
    rob->count++;
    return rob->tail - 1;  // Return index of the added entry
}

void commit_ROB_entry(ROB *rob, int *regs, int *zero_flag) {
    if (rob->count == 0) {
        return;  // ROB is empty
    }

    ROB_Entry *entry = &rob->entries[rob->head];
    if (entry->completed) {
        // Update architectural registers
        regs[entry->dest_arch_reg] = entry->value;

        if (entry->mispredicted) {
            *zero_flag = 0; // Reset zero flag
            printf("Branch misprediction at PC %d. Rolling back...\n", entry->pc);
        }

        // Advance ROB head
        rob->head = (rob->head + 1) % ROB_SIZE;
        rob->count--;
    }
}

void init_RenameTable(RenameTable *rename_table) {
    for (int i = 0; i < REG_FILE_SIZE; i++) {
        rename_table->arch_to_phys[i] = -1; // No mapping initially
    }
    for (int i = 0; i < PHYS_REG_COUNT; i++) {
        rename_table->phys_to_arch[i] = -1;
        rename_table->free_phys_regs[i] = i; // All physical registers are free
    }
    rename_table->free_count = PHYS_REG_COUNT;
}

int allocate_phys_reg(RenameTable *rename_table) {
    if (rename_table->free_count == 0) {
        return -1;  // No free physical registers
    }
    return rename_table->free_phys_regs[--rename_table->free_count];
}

void free_phys_reg(RenameTable *rename_table, int phys_reg) {
    rename_table->free_phys_regs[rename_table->free_count++] = phys_reg;
}


void init_BranchPredictor(BranchPredictor *bp) {
    for (int i = 0; i < 8; i++) {
        bp->history[i] = 1; // Initialize with weakly taken
    }
    bp->head = 0;
}

int predict_branch(BranchPredictor *bp) {
    return bp->history[bp->head] > 1; // Predict taken if history > 1
}

void update_branch(BranchPredictor *bp, int taken) {
    int *entry = &bp->history[bp->head];
    if (taken) {
        if (*entry < 3) (*entry)++;
    } else {
        if (*entry > 0) (*entry)--;
    }
    bp->head = (bp->head + 1) % 8;
}


/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/
        cpu->decode = cpu->fetch;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                break;
            }
        }

        /* Copy data from decode latch to execute latch*/
        cpu->execute = cpu->decode;
        cpu->decode.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }

            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->execute.result_buffer = cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            /* New Instructions */
            case OPCODE_LTR:
            {
                cpu->execute.result_buffer
                    = (cpu->execute.rs1_value < cpu->execute.rs2_value) ? 1 : 0;
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.imm;
                break;
            }

            case OPCODE_STORE:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs2_value + cpu->execute.imm;
                cpu->data_memory[cpu->execute.memory_address] = cpu->execute.rs1_value;
                break;
            }

            case OPCODE_JALP:
            {
                cpu->regs[cpu->execute.rd] = cpu->pc + 4; // Save return address
                cpu->pc = cpu->execute.pc + cpu->execute.imm; // Jump to target address
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;
                cpu->fetch.has_insn = TRUE;
                break;
            }

            case OPCODE_RET:
            {
                cpu->pc = cpu->regs[cpu->execute.rs1]; // Return to saved address in rs1
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;
                cpu->fetch.has_insn = TRUE;
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value * cpu->execute.rs2_value;
                break;
            }

            case OPCODE_NOP:
            {
                // Do nothing for NOP
                break;
            }

            case OPCODE_HALT:
            {
                printf("HALT encountered. Stopping simulation.\n");
                cpu->is_halted = TRUE; // Set halt flag
                break;
            }


        }

        /* Copy data from execute latch to memory latch */
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_ADD:
            {
                /* No work for ADD */
                break;
            }

            case OPCODE_LOAD:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                break;
            }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;
    init_ROB(&cpu->rob);
    init_RenameTable(&cpu->rename_table);
    init_BranchPredictor(&cpu->branch_predictor);
    cpu->is_halted = FALSE;




    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}




/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    if (cpu)
    {
        // Free dynamically allocated code_memory if it exists
        if (cpu->code_memory)
        {
            free(cpu->code_memory);
            cpu->code_memory = NULL;
        }

        // No need to free data_memory, as it's statically allocated
        free(cpu); // Free the CPU structure
    }
}

