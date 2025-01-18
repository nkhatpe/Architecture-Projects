/*
 * apex_macros.h
 * Contains APEX cpu pipeline macros
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _MACROS_H_
#define _MACROS_H_

#define FALSE 0x0
#define TRUE 0x1

/* Integers */
#define DATA_MEMORY_SIZE 4096

/* Size of integer register file */
#define REG_FILE_SIZE 16

/* Numeric OPCODE identifiers for instructions */
#define OPCODE_ADD 0x0
#define OPCODE_SUB 0x1
#define OPCODE_MUL 0x2
#define OPCODE_DIV 0x3
#define OPCODE_AND 0x4
#define OPCODE_OR 0x5
#define OPCODE_XOR 0x6
#define OPCODE_MOVC 0x7
#define OPCODE_LOAD 0x8
#define OPCODE_STORE 0x9
#define OPCODE_BZ 0xa
#define OPCODE_BNZ 0xb
#define OPCODE_HALT 0xc
/* New opcodes added */
#define OPCODE_CMP 0xd     /* Compare two registers */
#define OPCODE_CML 0xe     /* Compare register with literal */
#define OPCODE_JALR 0xf    /* Jump and link register */
#define OPCODE_JUMP 0x10   /* Jump */
#define OPCODE_LDR 0x11    /* Load register */
#define OPCODE_STR 0x12    /* Store register */
#define OPCODE_ADDL 0x13   /* Add register with literal */
#define OPCODE_SUBL 0x14   /* Subtract literal from register */
#define OPCODE_BP 0x15     /* Branch if positive */
#define OPCODE_BNP 0x16    /* Branch if not positive */
#define OPCODE_BN 0x17     /* Branch if negative */
#define OPCODE_NOP 0x18    /* No operation */

/* Set this flag to 1 to enable debug messages */
#define ENABLE_DEBUG_MESSAGES 1

/* Set this flag to 1 to enable cycle single-step mode */
#define ENABLE_SINGLE_STEP 1

#endif
