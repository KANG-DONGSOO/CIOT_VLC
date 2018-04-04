
.setcallreg  r29.w2		 // set a non-default CALL/RET register
.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back

// Constants from the MCP3004/3008 datasheet 
#define TIME_CLOCK      1       // T_hi and t_lo = 125ns = 25 instructions (min)

START:
        // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr

	MOV	r1, 0x00000000	 	// r1 = base address
//	LBBO    r2, r1, 0, 4    // r2 = delay of the clock
	MOV		r2, 82
	MOV	r3, r2				// r3 = counter for clock
	MOV	r4,	0				// r4 = Program state (0: Preamble - 1: SFD - 2: Payload)
	MOV r5, 170				// r5 = Preamble value (10101010)
	MOV	r6, 0				// r6 = Bit to be encoded
	MOV	r7, 0				// r7 = n_preamble
	MOV	r8, 8				// r8 = n_shift for preamble
	MOV	r9, 0				// r9 = temp for preamble value
	
	CLR	r30.t1			// * Clear SCLK
	CLR	r30.t2

PROCESSBEGIN:
	QBEQ	PREAMBLEBIT, r4, 0
	MOV		r4, 0
	QBA		PROCESSBEGIN

PREAMBLEBIT:
	SUB		r8, r8, 1
	LSR		r9, r5, r8
	AND		r6, r9, 1
	QBNE	ENCODEFIRST, r8, 0		// * While the preamble bits are not completely transmitted, proceed to bit encoding
	MOV		r8, 8
	ADD		r7, r7, 1
	QBNE	ENCODEFIRST, r7, 3
	MOV		r7, 0					// * [TODO] Increment Program state here after all preambles are sent
	QBA		ENCODEFIRST

ENCODEFIRST:
	SUB		r3, r3, 3				// * [TIMER ADJUSTMENT - DECREASE DURATION OF LOW DURING PREAMBLE]
	QBEQ	WRITEFIRSTHIGH, r6, 1	// * If the data is high, set output to LOW (and later HIGH)
	SET		r30.t2					// * Else, set output to HIGH (and later LOW)
	QBA		WAITHIGH
	
WRITEFIRSTHIGH:
	CLR		r30.t2

WAITHIGH:
	SUB		r3,	r3,	1				// * Decrement counter
	QBNE	WAITHIGH, r3, 0 		// * Loop if counter is not zero yet
	SET		r30.t1					// * SET the Clock bit
	MOV		r3, r2					// * Reset the delay value to clock counter
	QBEQ	WRITESECONDHIGH, r6, 1	// * If the data is high, set output to HIGH (as high is encoded as LOW - HIGH)
	CLR		r30.t2					// * else set output to LOW (as high is encoded as HIGH - LOW)
	QBA		WAITLOW
	
WRITESECONDHIGH:
	SET		r30.t2 

WAITLOW:
	SUB		r3, r3, 1				// * Decrement counter
	QBNE	WAITLOW, r3, 0			// * Loop if the counter is not zero yet
	CLR		r30.t1					// * Clear the Clock bit
	MOV		r3, r2					// * Reset the delay value to clock counter
	
	QBA		PROCESSBEGIN			// * Detect the next bit to be read		
	
END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"