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

START:	                         // aims to load the clock period value into r2
	MOV	r1, 0x00000000	 		// * r1 = base address
	MOV	r4, 0x00010000   		// * r4 = shared RAM address
	
	MOV	r2, 16					// * r2 = bit_shift
	MOV	r3, 0					// * r3 = cnt_symbol
	MOV	r4, 0					// * r4 = prev_symbol
	MOV	r5, 0					// * r5 = cnt_preamble
	MOV	r6, 0					// * r6 = curr_symbol
	MOV	r7, 0xAAAAAA				// * r7 = Input byte [DUMMY]
	MOV	r8, 0					// * r8 = Flag from PRU0 [TESTING]
	
//WAIT_FROM_OTHER_PRU:
//	LBBO r8, r4, 0, 1
//	QBBS DO_SOMETHING, r8.t0
//	QBA	WAIT_FROM_OTHER_PRU

//DO_SOMETHING:
//	QBBS CLEAR_OUTPUT, r30.t1
//	SET	r30.t1
	

	
START_OF_PROCESS:
	SET r30.t1
	MOV	r2, 16					// * r2 = bit_shift
	MOV	r3, 1					// - cnt_symbol = 1
	MOV	r5, 0					// - cnt_preamble = 0
	//MOV	r6, 0					// * r6 = curr_symbol
	MOV	r4, 0					// - prev_symbol = 0
	
DECODING_LOOP:
	QBEQ END_PREAMBLE_PROCESS, r2, 0
	SUB	r2,	r2,	1				// - bit_shift--
	LSR	r6, r7, r2				// - curr_symbol = input_byte>>bit_shift
	AND	r6, r6, 0x1				// - curr_symbol = curr_symbol & 1
	QBBC CHECK_CURR_HIGH, r3.t0	// - IF count_symbol == even THEN check if current_symbol_is_high_and_previous_is_low
	QBGT ADD_SYMBOL, r6, r4	// - OR count_symbol == odd THEN check if current_symbol_is_low_and_previous_is_high
	QBA RESET_PREAMBLE			// - ELSE reset the preamble detection
	
CHECK_CURR_HIGH:
	QBGT ADD_SYMBOL, r4, r6
	QBA	 RESET_PREAMBLE
	
ADD_SYMBOL:
	ADD	r3, r3, 1
	QBA	AFTER_ADD_SYMBOL

RESET_PREAMBLE:
	MOV	r3, 1
	MOV	r5, 0

AFTER_ADD_SYMBOL:
	MOV	r4, r6
	QBLE ADD_PREAMBLE, r3, 8
	QBA	DECODING_LOOP

ADD_PREAMBLE:
	MOV	r3, 0
	ADD	r5, r5, 1
	QBLE PREAMBLE_DONE, r5, PREAMBLE_LEN
	QBA	DECODING_LOOP

PREAMBLE_DONE:
	QBA	END_PREAMBLE_PROCESS

END_PREAMBLE_PROCESS:
	CLR	r30.t1
	QBA START_OF_PROCESS
	
END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT