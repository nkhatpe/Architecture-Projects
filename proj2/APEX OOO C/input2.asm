MOVC R0,#0		/* address of A[0] */
MOVC R1,#8		/* address of B[0] */
MOVC R2,#8		/* iteration count */
MOVC R3,#2		/* holds multiplier for A[i] */
LOAD R4,R0,#0 	/* R4 is assigned to X, holds A[i] */
JALP R5,#28		/* call calc_update */
STORE R4,R1,#0	/* updates B[i] */
ADDL R0,#4		/* point to next element of A */
ADDL R1,#4		/* point to next element of 8 */
SUBL R2,R2,#1		/* decrement loop counter */
BNZ  #-24		/* branch if not zero */
HALT
MUL R4,R4,R3		/* calc update starts here */
ADDL R4,R4,#1
RET R5
NOP
NOP
NOP
NOP
NOP
NOP