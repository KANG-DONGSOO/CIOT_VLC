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

#define PREAMBLE_STATE 0

START:	                         // aims to load the clock period value into r2
	MOV	r1, 0x00000000	 		// * r1 = base address
	LBBO	r2, r1, 4, 4	 	// * r2 = the global memory address to store result
	MOV	r3, 82					// * r3 = clock delay
	MOV	r4, 0x00010000   		// * r4 = shared RAM address
	MOV	r5, r3					// * r5 = counter for clock
	MOV	r6, 0					// * r6 = newly read bit
	MOV	r7,	0					// * r7 = current_bit
	MOV	r8,	0					// * r8 = current value
	MOV	r9, 0					// * r9 = state of the program (0 = Preamble | 1 = SFD )
	MOV	r10, 0					// * r10 = utility for decoding
	MOV	r11, 0					// * r11 = utility for decoding
	
	CLR	r30.t1
	
CLOCK_BEGIN:
	MOV	r5, r3					// * r5 = counter for clock
	
WAIT_FIRST:
	SUB	r5, r5, 1				// Decrease the clock counter value
	QBNE WAIT_FIRST, r5, 0		// While not zero, repeat the clock decrement
	LBBO r6, r4, 0, 1			// r6 <- Bit from shared memory (ADC Reading)
	MOV	r7, r6					// r7 <- r6 (Save newly read value to current bit)
	MOV	r5, r3					// Reset the clock counter
	//QBEQ CURRENTLOW, r6, 0	// [DEBUG]If r6 == LOW, store read_bit with LOW value
	//SET r30.t1				// [DEBUG]Checking ADC reading value
	//XOR	r30.w0, r30.w0, 2	// [DEBUG] checking clock rate of PRU OOK
	QBA WAIT_SECOND 			// Wait for second OOK character

//CURRENTLOW:
//	CLR r30.t1				// [DEBUG]Checking ADC reading value
//	XOR	r30.w0, r30.w0, 2	// [DEBUG] checking clock rate of PRU OOK

WAIT_SECOND:
	SUB	r5, r5, 1				// Decrease the clock counter value
	QBNE WAIT_SECOND, r5, 0		// While not zero, repeat the clock decrement
	LBBO r6, r4, 0, 1			// r6 <- Bit from shared memory (ADC Reading)
	QBNE DECODEOOK, r7, r6		// if newly read bit (r6) is NOT same with current_bit, then OOK is valid, go to decoding
	MOV	r5, r3					// else, restore the value of clock counter, wait for the second value of OOK again
	QBA WAIT_SECOND
		
DECODEOOK:
	QBEQ STORELOW, r6, 0		// If the newly read_bit is LOW, then the current_value is 0 (HIGH - LOW), store it
	//SET	r30.t1					// [DEBUG] Check if correctly reading OOK Decoded value
	MOV	r8, 1					// else, r8 <- 1 (LOW - HIGH)
	QBA PROCESS_BIT				// Repeat the process

STORELOW:
	//CLR	r30.t1					// [DEBUG] Check if correctly reading OOK Decoded value
	MOV	r8, 0					// r8 <- 0 (HIGH - LOW)
	QBA PROCESS_BIT
	
PROCESS_BIT:
	QBEQ PREAMBLE, r9, PREAMBLE_STATE //First, process Preamble (10101010 * 3 times)
	QBA	CLOCK_BEGIN
	
PREAMBLE:
	XOR r11, r8, r10
	QBBC PREAMBLERROR, r11.t0	//Since counter is from 0, the current bit of preamble must be the opposite of counter. Eq: preamble-0 is 1, counter is 0 (last bit = 0); preamble-1 is 0, counter is 1 (last bit = 1) etc
	ADD	r10, r10, 1
	QBEQ SFD, r10, 24
	QBA	CLOCK_BEGIN
	
PREAMBLERROR:
	MOV r10, 0						//Reset the preamble Bit Counter to 0 (Detect preamble from beginning again)
	QBA	CLOCK_BEGIN
	
SFD:
	//SET r30.t1
	//SET r30.t1
	//CLR	r30.t1
	//CLR	r30.t1
	QBA CLOCK_BEGIN	
END:
	HALT