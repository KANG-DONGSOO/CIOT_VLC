// PRU1 program to provide a variable frequency clock on P8_46 (pru1_pru_r30_1) 
// that is controlled from Linux userspace by setting the PRU memory state. This
// program is executed on PRU1 and outputs the sample clock on P8_46. If you wish
// to change the clock to output on a different pin edit the code. The program is
// memory controlled using the first two 4-byte numbers in PRU memory space:
// - The delay is set in memory address 0x00000000 (4 bytes long)
// - The counter can be turned on and off by setting the LSB of 0x00000004 (4 bytes) 
//   to 1 (on) or 0 (off)
// - The delay value can be updated by setting the second most LSB to 1 (it will 
//   immediately return to 1) e.g., set 0x00000004 to be 3, which will return to 1
//   to indicate that the update has been performed.
// This program was writen by Derek Molloy to align with the content of the book 
// Exploring BeagleBone, 2014. This example was written after publication.

.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back
#define PREAMBLE_STATE 0
#define PREAMBLE_LEN 3
#define NUM_OF_PROCESSED_BITS 16
#define MTU	1200

START:	                         // aims to load the clock period value into r2
	MOV	r1, 0x00000000	 		// * r1 = base address
	MOV	r4, 0x00010000   		// * r4 = shared RAM address
	
	MOV	r2, 0					// * r2 = current_bit (decoded value from PRU0)
	MOV	r3, 0					// * r3 = state (0 == SFD start, 1 == SFD during, 2 == Length Start, 2 == Payload, 3 == Final[?])
	MOV	r4, 0					// * r4 = cnt_symbol
	MOV	r5, 0					// * r5 = Temp value (SFD,length, payload, etc)
	MOV	r6, 0					// * r6 = temp value for logical shift
	MOV	r7, 0xAAAAAA				// * r7 = Input byte [DUMMY]
	MOV	r8, 0xa3				// * r8 = SFD value
	

	
START_OF_PROCESS:
	CALL	GET_CURRENT_BIT
	
CHECK_STATE:
	QBEQ	START_SFD, r3, 0
	QBEQ	PROCESS_SFD, r3, 1
	QBEQ	START_LENGTH, r3, 2
	QBEQ	PROCESS_LENGTH, r3, 3
	QBA		START_OF_PROCESS
	
START_SFD:
	MOV	r4, 7
	MOV	r5, 0
	ADD	r3, r3, 1
	QBA	CHECK_STATE
	
PROCESS_SFD:
	LSL	r6, r2, r4
	ADD	r5, r5, r6
	SUB	r4, r4, 1
	QBGT COMPARE_SFD, r4, 0
	QBA	START_OF_PROCESS

COMPARE_SFD:
	QBEQ AFTER_SFD, r5, r8
	MOV	r3, 0
	QBA	START_OF_PROCESS
	
AFTER_SFD:
	ADD	r3, r3, 1
	QBA	START_OF_PROCESS
	
START_LENGTH:
	MOV	r4, 15
	MOV	r5, 0
	ADD r3, r3, 1
	QBA CHECK_STATE
	
PROCESS_LENGTH:
	LSL	r6, r2, r4
	ADD	r5, r5, r6
	SUB	r4, r4, 1
	QBGT IS_LENGTH_VALID, r4, 0
	QBA	START_OF_PROCESS

IS_LENGTH_VALID:
	QBGE GOTOEND, r5, 0
	QBLT GOTOEND, r5, MTU
	//TODO: The value of length is valid, store it to send to kernel
	ADD	r3, r3, 1
	QBA	START_OF_PROCESS
	

END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT