/*
 * file_parser.c
 * Contains functions to parse input file and create code memory
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari
 * State University of New York at Binghamton
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Helper function to extract numbers from strings */
static int get_num_from_string(const char *buffer)
{
    char str[16];
    int i, j = 0;

    for (i = 1; buffer[i] != '\0'; ++i)
    {
        str[j] = buffer[i];
        j++;
    }
    str[j] = '\0';

    return atoi(str);
}

/* Sets the numeric opcode based on instruction string */
static int set_opcode_str(const char *opcode_str)
{
    if (strcmp(opcode_str, "ADD") == 0) return OPCODE_ADD;
    if (strcmp(opcode_str, "SUB") == 0) return OPCODE_SUB;
    if (strcmp(opcode_str, "MUL") == 0) return OPCODE_MUL;
    if (strcmp(opcode_str, "DIV") == 0) return OPCODE_DIV;
    if (strcmp(opcode_str, "AND") == 0) return OPCODE_AND;
    if (strcmp(opcode_str, "OR") == 0) return OPCODE_OR;
    if (strcmp(opcode_str, "EXOR") == 0) return OPCODE_XOR;
    if (strcmp(opcode_str, "MOVC") == 0) return OPCODE_MOVC;
    if (strcmp(opcode_str, "LOAD") == 0) return OPCODE_LOAD;
    if (strcmp(opcode_str, "STORE") == 0) return OPCODE_STORE;
    if (strcmp(opcode_str, "BZ") == 0) return OPCODE_BZ;
    if (strcmp(opcode_str, "BNZ") == 0) return OPCODE_BNZ;
    if (strcmp(opcode_str, "HALT") == 0) return OPCODE_HALT;
    /* New instructions */
    if (strcmp(opcode_str, "BP") == 0) return OPCODE_BP;
    if (strcmp(opcode_str, "CMP") == 0) return OPCODE_CMP;
    if (strcmp(opcode_str, "LTR") == 0) return OPCODE_LTR;
    if (strcmp(opcode_str, "JUMP") == 0) return OPCODE_JUMP;
    if (strcmp(opcode_str, "ADDL") == 0) return OPCODE_ADDL;
    if (strcmp(opcode_str, "SUBL") == 0) return OPCODE_SUBL;
    if (strcmp(opcode_str, "JALP") == 0) return OPCODE_JALP;
    if (strcmp(opcode_str, "RET") == 0) return OPCODE_RET;
    if (strcmp(opcode_str, "NOP") == 0) return OPCODE_NOP;

    printf("Unknown opcode: %s\n", opcode_str);
    assert(0 && "Invalid opcode");
    return 0;
}

static void
split_opcode_from_insn_string(char *buffer, char tokens[2][128])
{
    int token_num = 0;

    // Remove trailing newline or whitespace
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r' || buffer[len-1] == ' ')) {
        buffer[len-1] = '\0';
        len--;
    }

    char *token = strtok(buffer, " ");

    while (token != NULL && token_num < 2)
    {
        strcpy(tokens[token_num], token);
        token_num++;
        token = strtok(NULL, " ");
    }

    // Initialize the second token if not set
    if (token_num < 2) {
        tokens[1][0] = '\0';
    }
}

/* Creates APEX instruction structure from the assembly string */
static void create_APEX_instruction(APEX_Instruction *ins, char *buffer)
{
    int i, token_num = 0;
    char tokens[6][128];
    char top_level_tokens[2][128];

    for (i = 0; i < 2; ++i)
    {
        strcpy(top_level_tokens[i], "");
    }

    split_opcode_from_insn_string(buffer, top_level_tokens);

    char *token = strtok(top_level_tokens[1], ",");

    while (token != NULL)
    {
        strcpy(tokens[token_num], token);
        token_num++;
        token = strtok(NULL, ",");
    }

    strcpy(ins->opcode_str, top_level_tokens[0]);
    ins->opcode = set_opcode_str(ins->opcode_str);

    switch (ins->opcode)
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
            ins->rd = get_num_from_string(tokens[0]);
            ins->rs1 = get_num_from_string(tokens[1]);
            ins->rs2 = get_num_from_string(tokens[2]);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            ins->rd = get_num_from_string(tokens[0]);
            ins->rs1 = get_num_from_string(tokens[1]);
            ins->imm = get_num_from_string(tokens[2]);
            break;
        }

        case OPCODE_MOVC:
        {
            ins->rd = get_num_from_string(tokens[0]);
            ins->imm = get_num_from_string(tokens[1]);
            break;
        }

        case OPCODE_LOAD:
        {
            ins->rd = get_num_from_string(tokens[0]);
            ins->rs1 = get_num_from_string(tokens[1]);
            ins->imm = get_num_from_string(tokens[2]);
            break;
        }

        case OPCODE_STORE:
        {
            ins->rs1 = get_num_from_string(tokens[0]);
            ins->rs2 = get_num_from_string(tokens[1]);
            ins->imm = get_num_from_string(tokens[2]);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        {
            ins->imm = get_num_from_string(tokens[0]);
            break;
        }
        
        case OPCODE_CMP:
        {
            ins->rs1 = get_num_from_string(tokens[0]);
            ins->rs2 = get_num_from_string(tokens[1]);
            break;
        }
        
        case OPCODE_JUMP:
        {
            ins->rs1 = get_num_from_string(tokens[0]);
            ins->imm = get_num_from_string(tokens[1]);
            break;
        }

        case OPCODE_JALP:
        {
            ins->rd = get_num_from_string(tokens[0]);
            ins->imm = get_num_from_string(tokens[1]);
            break;
        }

        case OPCODE_RET:
        {
            ins->rs1 = get_num_from_string(tokens[0]);
            break;
        }
    }
}

/* Creates code memory from the input file */
APEX_Instruction *create_code_memory(const char *filename, int *size)
{
    FILE *fp;
    ssize_t nread;
    size_t len = 0;
    char *line = NULL;
    int code_memory_size = 0;
    int current_instruction = 0;
    APEX_Instruction *code_memory;

    if (!filename)
    {
        return NULL;
    }

    fp = fopen(filename, "r");
    if (!fp)
    {
        return NULL;
    }

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        code_memory_size++;
    }
    *size = code_memory_size;
    if (!code_memory_size)
    {
        fclose(fp);
        return NULL;
    }

    code_memory = calloc(code_memory_size, sizeof(APEX_Instruction));
    if (!code_memory)
    {
        fclose(fp);
        return NULL;
    }

    rewind(fp);
    while ((nread = getline(&line, &len, fp)) != -1)
    {
        create_APEX_instruction(&code_memory[current_instruction], line);
        current_instruction++;
    }

    free(line);
    fclose(fp);
    return code_memory;
}
