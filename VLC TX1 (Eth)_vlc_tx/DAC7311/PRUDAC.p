.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back

START:
        // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr

	MOV	r1, 0x00000000	 	// r1 = base address
	LBBO    r2, r1, 0, 4    // r2 = delay of the clock
	MOV		r3,	0			// r3 = counter for clock
	MOV	r4,	0				// r4 = input shift iterator
	MOV r5, 0				// r5 = Debug register (Store the value of sent bits)
	MOV	r6, 0
	
	SET	r30.t1			// * Set SCLK

CLOCKBEGIN:
	MOV	r3, r2			// * Store the delay value to clock counter
	ADD	r3,	r3,	1		// * Balance duty cycle by looping 1 extra time on High

WAITLOW:
	SUB		r3,	r3,	1			// * Decrement counter
	QBNE	WAITLOW, r3, 0 		// * Loop if counter is not zero yet
	CLR		r30.t1
	MOV	r3, r2					// * Reset the delay value to clock counter
	
WAITHIGH:
	SUB		r3, r3, 1			// * Decrement counter
	QBEQ	PROCSGNL, r3, 3		// * In the middle of Clock LOW, process signal for SYNC and Din
	QBNE	WAITHIGH, r3, 0		// * Loop if the counter is not zero yet
	SET		r30.t1				// * Set SCLK
	QBA		CLOCKBEGIN			// * Reset the clock again

PROCSGNL:
	QBEQ	BEGINSEQ, r4, 0		// * If input hasn't started yet (Input shift iterator = 0), begin sequence
	QBGT	SETMODE, r4, 3		// * For the first 2 characters, set the mode of DAC
	QBEQ	ENDSEQ, r4, 17		// * After the 16th characters, end the sequence by SET the SYNC
								// * For 1st - 16th characters, input the actual data
	QBEQ	DATASET, r6, 1
	CLR		r30.t2
	LSL		r5, r5, 1

AFTERDATA:	
	ADD		r4, r4, 1
	QBA		WAITHIGH

DATASET:
	SET		r30.t2				// * [DUMMY]Currently only writing HIGH to the Data Input Register
	LSL		r5, r5, 1
	OR		r5, r5, 1
	QBA		AFTERDATA
	
BEGINSEQ:
	QBBS	CLRSEQ, r30.t5		// * If sync is currently SET, Clear it and Process next signal
	SET		r30.t5				// * else, Set the sync and wait until 1 clock cycle
	QBA		WAITHIGH
	

CLRSEQ:
	CLR		r30.t5
	ADD		r4, r4, 1
	QBA		PROCSGNL
	
SETMODE:
	CLR		r30.t2				// * For the default mode, both of mode bits are LOW
	LSL		r5, r5, 1
	OR		r5,	r5, 0
	ADD		r4, r4, 1
	QBA		WAITHIGH
	
ENDSEQ:
	SET		r30.t5
	MOV		r4, 0
	XOR		r6, r6, 1
	QBA		WAITHIGH
	
END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"