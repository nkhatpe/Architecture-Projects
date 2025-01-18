# APEX Pipeline Simulator v2.0
A template for 5 Stage APEX In-order Pipeline

## Notes:

 - This code is a simple implementation template of a working 5-Stage APEX In-order Pipeline
 - Implementation is in `C` language
 - Stages: Fetch -> Decode -> Execute -> Memory -> Writeback
 - You can read, modify and build upon given code-base to add other features as required in project description
 - You are also free to write your own implementation from scratch
 - All the stages have latency of one cycle
 - There is a single functional unit in Execute stage which perform all the arithmetic and logic operations
 - Logic to check data dependencies has not be included
 - Includes logic for `ADD`, `LOAD`, `BZ`, `BNZ`,  `MOVC` and `HALT` instructions
 - On fetching `HALT` instruction, fetch stage stop fetching new instructions
 - When `HALT` instruction is in commit stage, simulation stops
 - You can modify the instruction semantics as per the project description

## Files:

 - `Makefile`
 - `file_parser.c` - Functions to parse input file
 - `apex_cpu.h` - Data structures declarations
 - `apex_cpu.c` - Implementation of APEX cpu
 - `apex_macros.h` - Macros used in the implementation
 - `main.c` - Main function which calls APEX CPU interface
 - `input.asm` - Sample input file

## How to compile and run

 Go to terminal, `cd` into project directory and type:
```
 make
```
 Run as follows:
```
 ./apex_sim <input_file_name>
```

Narendra Khatpe: B00984858

Below are the instructions how to use the simulator 

1) After running as above, you will see a simulator command interface as follows

2) Available commands:
	  SetMem <dfilename> - Initialize data memory
	  Initialize - Initialize simulator state
	  Simulate <n> - Simulate n cycles
	  Single_step - Advance simulation by one cycle
	  Display - Show pipeline, registers, and memory state
	  ShowMem <address> - Display content of specific memory location
	  Exit - Quit the simulator
  
  	  Enter command: 
  
3)	  You can enter the command as per your choice
	  
	  For instance:
	  
	  simulate followed by number '10' '20' '30' etc
	  setmem <test-data file name> I have added sample test_data.txt and its working as expected!

Information from the author!
