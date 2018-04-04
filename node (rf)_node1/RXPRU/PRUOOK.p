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

START:	                         // aims to load the clock period value into r2
	MOV	r1, 0x00000000	 		// * r1 = base address
	MOV	r4, 0x00010000   		// * r4 = shared RAM address
	MOV	r2, 0					// * r2 = Flag from RXPRU
	MOV	r3, 0					// * r3 = 16bits from RX PRU
	MOV	r5, 0					// * r5 = Bit Counter
	MOV	r6, 0					// * r6 = temp, shift result value from bits	
	MOV	r7, 0					// * r7 = STATE OF ENCODING (Preamble, SFD, DATA)
	MOV	r10, 0
	MOV	r12, 1
	
	//SBBO	r2, r4, 0, 2
	MOV	r13, 0
	MOV r14, 0
	MOV	r15, 0
	MOV	r16, 0
	MOV	r17, 0
	MOV	r18, 0
	MOV	r19, 0
	
	CLR	r30.t1
	
//DUMMY DATA FOR ENCODING TESTING
	MOV 	r9, 0x6666
	SBBO	r9, r1, 0, 2
	MOV		r9, 0x66a5
	SBBO	r9, r1, 2, 2
//	MOV		r9, 0xaaaaaa9a
//	SBBO	r9, r4, 4, 4
//	MOV		r9, 0xaaaaaaa9
//	SBBO	r9, r4, 8, 4
//	MOV		r9, 0xaaaaa9aa
//	SBBO	r9, r4, 12, 4
//	MOV		r9, 0x66666666
//	SBBO	r9, r4, 16, 4
//	SBBO	r9, r4, 20, 4
//	SBBO	r9, r4, 24, 4
//	SBBO	r9, r4, 28, 4
//	SBBO	r9, r4, 32, 4

//ENCODE:
//	CALL	GET_2_BYTE			// - Call function to read 2 byte of encoded data and save it to register
//	MOV		r5, 16				// - Set Bit counter to 16
	
//PROCESSBITS:
//	SUB		r5, r5, 1
//	LSR		r6, r3, r5
//	CALL	ENCODE_BIT
//	QBNE	PROCESSBITS, r5, 0
//	QBA		ENCODE

CHECK_PROC_FLAG:
	LBBO	r11, r4, 1, 1
	QBBS	CHECK_PROC_FLAG, r11.t0
	
WAITFLAG:
	LBBO	r2, r4, 0, 1
	QBBC	WAITFLAG, r2.t0
	CALL	TEST_DECODE
	SBBO	r12, r4, 1, 1
	QBA		CHECK_PROC_FLAG
//	LBBO	r3, r4, 1, 2
//	//SET		r30.t1
	
//PROCESSBITS:
//	SUB		r5, r5, 1
//	LSR		r6, r3, r5
//	QBBC	DEBUGCLEAR, r6.t0
//	ADD		r8, r8, 1
	//SET		r30.t1
//	QBA		AFTERPROCESS
	
	
//DEBUGCLEAR:
	//CLR		r30.t1
//	ADD		r7, r7, 1

//AFTERPROCESS:
//	QBNE	PROCESSBITS, r5, 0
//	MOV		r5, 16
	//CLR		r30.t1
//	QBBC	SETCLK, r30.t1
//	CLR		r30.t1
//	QBA		WAITFLAG

//SETCLK:
//	SET		r30.t1
//	QBA 	WAITFLAG	

END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT
	
//GET_2_BYTE:
//	LBBO	r3, r4, r10, 2
//	QBEQ	RESET_BIT_OFFSET, r10, 34
//	ADD		r10, r10, 2
//	RET

//RESET_BIT_OFFSET:
//	MOV		r10, 0
//	RET
	
TEST_DECODE:
	SET	r30.t1
	//DECODING TEST HAPPENS HERE
	MOV		r5, 16				// - Set Bit counter to 16
	MOV		r13, 0
	MOV		r19, 0
	
PROCESS_BIT:	
	SUB		r5, r5, 1
	//OOK BIT DECODING IS DONE HERE

CHECK_STATE:
	QBEQ	GET_SECOND_BIT, r16, 1
	//state = 0, GET FIRST BIT
	CALL	GET_BIT
	MOV		r14, r18
	MOV		r16, 1		//	*	First bit obtained, move state to 1 (to get second bit)
	QBA		AFTER_BIT
	
GET_SECOND_BIT:
	CALL	GET_BIT
	MOV		r15, r18
	QBEQ	GET_SECOND_BIT, r14, r15	//	* IF current_bit == last_bit, this means the phase of OOK incorrect, Shift it 1
	LSL		r13, r13, 1
	QBEQ	HIGH_BIT, r14, 1
	QBA		AFTER_BIT
	
HIGH_BIT:
	OR		r13, r13, 0x1
	
AFTER_BIT:
	QBNE	PROCESS_BIT, r5, 0
	CLR	r30.t1
	RET
	
GET_BIT:
	LBBO	r18, r1, r19, 1
	QBEQ	RESET_OFFSET, r19, 15
	ADD		r19, r19, 1
	RET
	
RESET_OFFSET:
	MOV		r19, 0
	RET