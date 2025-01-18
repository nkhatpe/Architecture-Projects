# CPU Pipeline Simulators

This repository contains implementations of cycle-by-cycle simulators for APEX (Application Processing Executive) pipeline architectures, developed for Computer Architecture coursework at SUNY Binghamton.

## Projects Overview

### 1. In-Order Pipeline Simulator
- Implementation of a 6-stage APEX pipeline with in-order execution
- Features:
  - Simple scoreboarding for dependency handling
  - Data forwarding mechanism
  - Cycle-by-cycle simulation
  - Memory operations with two-stage execution (MEM1, MEM2)

#### Pipeline Stages
- Fetch (F)
- Decode/Register File (D/RF)
- Execute (EX)
- Memory 1 (MEM1)
- Memory 2 (MEM2)
- Writeback (WB)

### 2. Out-of-Order Pipeline Simulator
- Built from scratch implementation of an out-of-order execution pipeline
- Advanced architectural features for improved performance
- [Note: Original implementation details to be updated]

## Instruction Set Architecture (ISA)

### Supported Instructions
- Arithmetic: `ADD`, `ADDL`, `SUB`, `SUBL`, `MUL`
- Memory: `LOAD`, `STORE`, `LDR`, `STR`
- Logic: `AND`, `OR`, `EX-OR`
- Control: `BZ`, `BNZ`, `BP`, `BNP`, `BN`, `JUMP`, `JALR`
- Data Movement: `MOVC`
- Comparison: `CML`, `CMP`
- System: `HALT`, `NOP`

### Features
- 32 general-purpose registers (R0-R31)
- Special CC register for flags (Z, N, P)
- 4-byte instruction and data words
- PC-relative addressing for branches

## Simulator Commands
- `initialize` - Reset simulator state
- `simulate <n>` - Run simulation for n cycles
- `single_step` - Execute one cycle
- `display` - Show pipeline state and registers
- `show mem <addr>` - Display memory contents
- `setmem <filename>` - Initialize data memory

## Technical Implementation
- Written in C for optimal performance
- Memory simulation for both instructions and data
- Cycle-accurate execution tracking
- Dependency handling through scoreboarding
- Data forwarding support in stage D/RF

## Academic Context
These projects were completed as part of CS 520 - Computer Architecture at SUNY Binghamton. They demonstrate understanding of:
- Pipeline architectures
- Instruction level parallelism
- Data hazard handling
- Control flow implementation
- Memory system design

## Testing
- Comprehensive test cases for individual instructions
- Full program execution tests
- Edge case handling verification
- Performance analysis capabilities

---
*Note: The out-of-order pipeline implementation was built from scratch and the original files are not currently available. This section will be updated when the implementation details are recovered.*
