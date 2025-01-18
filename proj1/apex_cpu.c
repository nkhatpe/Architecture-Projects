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
#include <stdbool.h>

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
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
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
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
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
void
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

void print_pipeline_state(const APEX_CPU *cpu)
{
    printf("--------------------------------------------\n");
    printf("Pipeline State:\n");
    printf("--------------------------------------------\n");
    print_stage_content("Fetch", &cpu->fetch);
    print_stage_content("Decode", &cpu->decode);
    print_stage_content("Execute", &cpu->execute);
    print_stage_content("Memory", &cpu->memory);
    print_stage_content("Writeback", &cpu->writeback);
    printf("--------------------------------------------\n");
}

/* 
 * This function prints the contents of the specified memory range.
 */
void print_memory(const APEX_CPU *cpu, int start_address, int num_locations)
{
    printf("--------------------------------------------\n");
    printf("Memory Contents:\n");
    printf("--------------------------------------------\n");
    for (int i = start_address; i < start_address + num_locations && i < DATA_MEMORY_SIZE; i++)
    {
        printf("Memory[%d] = %d\n", i, cpu->data_memory[i]);
    }
    printf("--------------------------------------------\n");
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
        bool stall = false;
        int rs1_value = 0;
        int rs2_value = 0;

        /* Handle source register 1 (rs1) */
        if (cpu->decode.rs1 != -1)
        {
            if (cpu->execute.has_insn && cpu->execute.rd == cpu->decode.rs1)
            {
                if (cpu->execute.opcode == OPCODE_LOAD || cpu->execute.opcode == OPCODE_LDR)
                {
                    stall = true;  // Must stall for loads
                }
                else
                {
                    rs1_value = cpu->execute.result_buffer;
                }
            }
            else if (cpu->memory.has_insn && cpu->memory.rd == cpu->decode.rs1)
            {
                rs1_value = cpu->memory.result_buffer;
            }
            else if (cpu->writeback.has_insn && cpu->writeback.rd == cpu->decode.rs1)
            {
                rs1_value = cpu->writeback.result_buffer;
            }
            else if (cpu->scoreboard[cpu->decode.rs1])
            {
                stall = true;
            }
            else
            {
                rs1_value = cpu->regs[cpu->decode.rs1];
            }
        }

        /* Handle source register 2 (rs2) */
        if (cpu->decode.rs2 != -1)
        {
            if (cpu->execute.has_insn && cpu->execute.rd == cpu->decode.rs2)
            {
                if (cpu->execute.opcode == OPCODE_LOAD || cpu->execute.opcode == OPCODE_LDR)
                {
                    stall = true;  // Must stall for loads
                }
                else
                {
                    rs2_value = cpu->execute.result_buffer;
                }
            }
            else if (cpu->memory.has_insn && cpu->memory.rd == cpu->decode.rs2)
            {
                rs2_value = cpu->memory.result_buffer;
            }
            else if (cpu->writeback.has_insn && cpu->writeback.rd == cpu->decode.rs2)
            {
                rs2_value = cpu->writeback.result_buffer;
            }
            else if (cpu->scoreboard[cpu->decode.rs2])
            {
                stall = true;
            }
            else
            {
                rs2_value = cpu->regs[cpu->decode.rs2];
            }
        }

        /* Check instruction-specific dependencies */
        if (!stall)
        {
            switch (cpu->decode.opcode)
            {
                case OPCODE_ADD:
                case OPCODE_SUB:
                case OPCODE_MUL:
                case OPCODE_AND:
                case OPCODE_OR:
                case OPCODE_XOR:
                case OPCODE_LDR:
                {
                    /* These instructions need both source registers */
                    if (cpu->decode.rs1 != -1 && cpu->decode.rs2 != -1)
                    {
                        cpu->decode.rs1_value = rs1_value;
                        cpu->decode.rs2_value = rs2_value;
                    }
                    break;
                }

                case OPCODE_LOAD:
                case OPCODE_STORE:
                {
                    /* These instructions only need rs1 */
                    if (cpu->decode.rs1 != -1)
                    {
                        cpu->decode.rs1_value = rs1_value;
                    }
                    break;
                }

                case OPCODE_STR:
                {
                    /* STR needs both rs1 (base) and rs2 (value to store) */
                    if (cpu->decode.rs1 != -1 && cpu->decode.rs2 != -1)
                    {
                        cpu->decode.rs1_value = rs1_value;
                        cpu->decode.rs2_value = rs2_value;
                    }
                    break;
                }

                case OPCODE_ADDL:
                case OPCODE_SUBL:
                case OPCODE_CMP:
                case OPCODE_CML:
                {
                    /* These instructions only need rs1 */
                    if (cpu->decode.rs1 != -1)
                    {
                        cpu->decode.rs1_value = rs1_value;
                    }
                    break;
                }

                case OPCODE_MOVC:
                {
                    /* MOVC doesn't need any source registers */
                    break;
                }

                case OPCODE_JUMP:
                {
                    /* JUMP needs rs1 for target address calculation */
                    if (cpu->decode.rs1 != -1)
                    {
                        cpu->decode.rs1_value = rs1_value;
                    }
                    break;
                }

                case OPCODE_JALR:
                {
                    /* JALR needs rs1 for target address calculation */
                    if (cpu->decode.rs1 != -1)
                    {
                        cpu->decode.rs1_value = rs1_value;
                    }
                    break;
                }

                case OPCODE_BZ:
                case OPCODE_BNZ:
                case OPCODE_BP:
                case OPCODE_BNP:
                case OPCODE_BN:
                {
                    /* Branch instructions don't need register values */
                    break;
                }
            }
        }

        if (!stall)
        {
            /* Mark destination register as busy in scoreboard */
            if (cpu->decode.rd != -1 && 
                cpu->decode.opcode != OPCODE_STORE && 
                cpu->decode.opcode != OPCODE_STR)
            {
                cpu->scoreboard[cpu->decode.rd] = 1;
            }

            /* Copy instruction to execute stage */
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = false;

            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("Decode/RF", &cpu->decode);
            }
        }
        else
        {
            if (ENABLE_DEBUG_MESSAGES)
            {
                printf("Decode: Instruction stalled\n");
            }
        }
    }
}
/*
 * Execute Stage of APEX Pipeline
 *
 * Handles the logic for executing various instructions in the EX stage.
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
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
            printf("DEBUG: ADD R%d = R%d(%d) + R%d(%d) = %d\n", 
                cpu->execute.rd, cpu->execute.rs1, cpu->execute.rs1_value,
                cpu->execute.rs2, cpu->execute.rs2_value, cpu->execute.result_buffer);
            break;
        }

            case OPCODE_SUB:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
                break;
            }

            case OPCODE_ADDL: /* Add register with literal */
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->zero_flag = (cpu->execute.result_buffer == 0);
                break;
            }

            case OPCODE_SUBL: /* Subtract literal from register */
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
                cpu->zero_flag = (cpu->execute.result_buffer == 0);
                break;
            }

            /* Logical Instructions */
            case OPCODE_AND:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;
                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
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

            case OPCODE_CMP:
            {
                
                /* Set the zero flag based on the result buffer */
                if (cpu->execute.rs1_value == cpu->execute.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }

                if (cpu->execute.rs1_value < cpu->execute.rs2_value)
                {
                    cpu->negative_flag = TRUE;
                } 
                else 
                {
                    cpu->negative_flag = FALSE;
                }

                if (cpu->execute.rs1_value > cpu->execute.rs2_value)
                {
                    cpu->positive_flag = TRUE;
                } 
                else 
                {
                    cpu->positive_flag = FALSE;
                }

                break;
            }

            case OPCODE_CML: /* Compare register with literal */
            {
                int result = cpu->execute.rs1_value - cpu->execute.imm;
                cpu->zero_flag = (result == 0);
                cpu->negative_flag = (result < 0);
                cpu->positive_flag = (result > 0);
                break;
            }

            /* Branching Instructions */
            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->decode.has_insn = FALSE;
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

            case OPCODE_BP: /* Branch if positive */
            {
                if (cpu->positive_flag == TRUE)
                {
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->decode.has_insn = FALSE;
                }
                break;
            }

            case OPCODE_BNP: /* Branch if not positive */
            {
                if (cpu->positive_flag == FALSE)
                {
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->decode.has_insn = FALSE;
                }
                break;
            }

            case OPCODE_BN: /* Branch if negative */
            {
                if (cpu->negative_flag == TRUE)
                {
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->decode.has_insn = FALSE;
                }
                break;
            }

            /* Control Flow Instructions */
            case OPCODE_JALR: /* Jump and link register */
            {
                cpu->regs[cpu->execute.rd] = cpu->pc;  // Save the return address
                cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;  // Jump to target
                cpu->fetch_from_next_cycle = TRUE;  // Stall fetch to use new PC
                break;
            }

            case OPCODE_JUMP: /* Unconditional jump */
            {
                cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;  // Jump to target
                cpu->fetch_from_next_cycle = TRUE;  // Stall fetch to use new PC
                break;
            }

            /* Memory Instructions */
            case OPCODE_LOAD:
            case OPCODE_LDR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
                break;
            }


            case OPCODE_STORE:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.result_buffer = cpu->regs[cpu->execute.rd];  // Store R2's value
                
                if (ENABLE_DEBUG_MESSAGES)
                {
                    printf("Execute: STORE Mem[%d] <- R%d = %d\n", 
                        cpu->execute.memory_address,
                        cpu->execute.rd,
                        cpu->regs[cpu->execute.rd]);  // Use actual register value
                }
                break;
            }
            case OPCODE_STR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }

            /* Miscellaneous Instructions */
            case OPCODE_NOP:
            case OPCODE_HALT:
            {
                // NOP and HALT are already handled
                break;
            }
        }

        /* Update flags */
        cpu->zero_flag = (cpu->execute.result_buffer == 0);
        cpu->positive_flag = (cpu->execute.result_buffer > 0);
        cpu->negative_flag = (cpu->execute.result_buffer < 0);

        /* Copy data from execute latch to memory latch */
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
        printf("Execute: Instruction %s, Destination register R%d\n", 
               cpu->execute.opcode_str, cpu->execute.rd);
        printf("Scoreboard status: R%d is marked as in use\n", cpu->execute.rd);
        }
    }
}


/*
 * Memory Stage of APEX Pipeline
 *
 * Handles memory operations like LOAD, STORE, LDR, and STR.
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_LOAD:
            case OPCODE_LDR:
            {
                // Calculate effective address
                int effective_address = cpu->memory.memory_address;
                
                // Perform the load operation
                cpu->memory.result_buffer = cpu->data_memory[effective_address];
                
                if (ENABLE_DEBUG_MESSAGES)
                {
                    printf("Memory: LOAD R%d <- Mem[%d] = %d\n", 
                           cpu->memory.rd, effective_address, cpu->memory.result_buffer);
                }
                break;
            }

            case OPCODE_STORE:
            {
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.result_buffer;
                
                if (ENABLE_DEBUG_MESSAGES)
                {
                    printf("Memory: STORE Mem[%d] <- %d\n",
                        cpu->memory.memory_address,
                        cpu->memory.result_buffer);
                }
                break;
            }
            case OPCODE_STR:
            {
                // Calculate effective address
                int effective_address = cpu->memory.memory_address;
                
                // Perform the store operation
                cpu->data_memory[effective_address] = cpu->memory.rs1_value;
                
                if (ENABLE_DEBUG_MESSAGES)
                {
                    printf("Memory: STORE Mem[%d] <- R%d = %d\n", 
                           effective_address, cpu->memory.rs1, cpu->memory.rs1_value);
                }
                break;
            }

            default:
                // Other instructions just pass through
                break;
        }

        // Move instruction to writeback stage
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = false;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Writes results back to the register file or stops the simulation if a HALT instruction is encountered.
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
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                printf("DEBUG: Writeback - R%d = %d\n", cpu->writeback.rd, cpu->writeback.result_buffer);
                break;
            }

            case OPCODE_CMP:  /* CMP doesn't write result but updates flags */
            case OPCODE_CML:  /* CML doesn't write result but updates flags */

            case OPCODE_MOVC: 
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_ADDL:  /* Add literal to register */
            case OPCODE_SUBL:  /* Subtract literal from register */
            {
                /* Write result of ADDL/SUBL to destination register */
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOAD:  /* Load from memory */
            case OPCODE_LDR:
            {
                /* Write the loaded value to destination register */
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }


            case OPCODE_STORE: /* Store does not write back to register file */
            case OPCODE_STR:   /* Store register-based does not write back */
            {
                /* No writeback for STORE or STR as they update memory */
                break;
            }

            case OPCODE_JALR:  /* Jump and link register */
            {
                /* JALR saves the return address to rd */
                cpu->regs[cpu->writeback.rd] = cpu->writeback.pc + 4;
                break;
            }

            case OPCODE_NOP:
            {
                /* NOP: No operation, no writeback */
                break;
            }
        }

        // Clear scoreboard entry
        if (cpu->writeback.opcode != OPCODE_STORE && cpu->writeback.opcode != OPCODE_STR && cpu->writeback.rd != -1)
        {
            cpu->scoreboard[cpu->writeback.rd] = 0;
            
            if (ENABLE_DEBUG_MESSAGES)
            {
                printf("Writeback: Clearing scoreboard for R%d\n", cpu->writeback.rd);
            }
        }

        /* Instruction completed, increment counter */
        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        /* Halt the simulation if a HALT instruction is encountered */
        if (cpu->writeback.opcode == OPCODE_HALT)
        {
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
    /* Initialize scoreboard */
    memset(cpu->scoreboard, 0, sizeof(int) * REG_FILE_SIZE);

    /* Preload memory at address 104 with value 42 */
    cpu->data_memory[104] = 42;

    cpu->single_step = ENABLE_SINGLE_STEP;

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
int APEX_cpu_run_single_cycle(APEX_CPU *cpu)
{
    if (cpu->clock < 1)
    {
        printf("APEX_CPU: Simulation Started\n");
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        printf("--------------------------------------------\n");
        printf("Clock Cycle #: %d\n", cpu->clock);
        printf("--------------------------------------------\n");
    }

    if (APEX_writeback(cpu))
    {
        // HALT instruction encountered
        printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
        return TRUE;
    }
    
    APEX_memory(cpu);
    APEX_execute(cpu);
    APEX_decode(cpu);
    APEX_fetch(cpu);

    if (ENABLE_DEBUG_MESSAGES)
    {
        print_reg_file(cpu);
    }

    cpu->clock++;
    return FALSE;
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}