
.setcallreg  r29.w2		 // set a non-default CALL/RET register
.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back
#define PREAMBLE_LEN 3

START:
	//INITIALIZATION
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     // * load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // * clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // * store the modified r0 back at the load addr
	
	//SETUP GPI FOR BIT-SHIFT INPUT
	LBCO	r0, C4, 8, 4	// * load SYSCFG reg into r0 (use c4 const addr)
	LDI		r0.b0, 0x82		// 0-1: 0x2 (01), 2: 0x0 (0), 3-7: 0x15 (10101)
	LDI		r0.b1, 0x2		// 8-12: 0x2 (01)
	SBCO	r0, C4, 8, 4	// * store the modified r0 back to the CFG register
	
	MOV		r0, 0x00000000	// r0: PRU0 Base Address | Low Value
	MOV		r1, 0x00010000	// r1: Shared RAM Address
	MOV		r2, 1			// r2: High Value
	MOV		r3, 0			// r3: 16 bit input value
	MOV		r4, 0			// r4: Program State (0 = During Preamble Detection, 1 = OOK Decoding)
	MOV		r5, 0			// r5: 16 time counter, to do the looping for processing each and every bit
	MOV		r6, 0			// r6: N-times shift, to get currently processed bit from the 16 bit input
	MOV		r7, 0			// r7: Value of currently processed bit (HIGH or LOW)
	//Preamble Detection Variables
	MOV		r8, 0			// r8: cnt_symbol
	MOV		r9, 0			// r9: cnt_preamble
	MOV		r10, 0			// r10: prev_symbol
	MOV		r11, 16			// r11: number of bits (16)

	//Preamble Detection Initialization
	MOV	r8, 1					// * cnt_symbol <- 1
	MOV	r9, 0					// * cnt_preamble <- 0
	MOV	r10, 0					// * prev_symbol <- 0
	
	//WAIT UNTIL START BIT IS SET
	WBS		r31.t29

READDATA:
	QBBC	CLEAR, r31.t28		// * IF Cnt_16 == Clear THEN wait ELSE DO PROCESSING
	//SBBO	r31, r0, 0, 4		// * [DEBUG] Save value of Register 31 to memory for value checking
	SET		r30.t2			// * [DEBUG] Output State of Cnt_16 for debug
	//MOV		r3, r31.w2			// * r3 <- last 16 bit of r31 (*** From current checking, the last 16 bit is stored in this region with LSB ORDER. Please adjust accordingly if its incorrect)
	
	MOV		r5, 48				// * r5 <- 16, program will loop 16 times to process all bits individually
	
WAITING:
	SUB		r5, r5, 1
	QBNE	WAITING, r5, 0
	QBA		READDATA
	
	
CLEAR:
	CLR		r30.t2			// * [DEBUG] Output state of GPI_SB for debug
	QBA READDATA

END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"