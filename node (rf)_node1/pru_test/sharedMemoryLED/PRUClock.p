.origin 0
.entrypoint START

#define INS_PER_US   200         // 5ns per instruction
#define INS_PER_DELAY_LOOP 2     // two instructions per delay loop
                                 // set up a 50ms delay
#define DELAY  50 * 1000 * (INS_PER_US / INS_PER_DELAY_LOOP) 

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back

START:
	MOV		r1, 0x00000000
	MOV	 	r2,	DELAY
	MOV 	r4, 0x00010000
	MOV		r5,	r1

MAINLOOP:
	SET 	r30.t1
	SET		r5.t00
	SBBO	r5,	r4,	0,	1
	MOV		r0,	r2
	ADD		r0,	r0,	1
	
DELAYON:
	SUB		r0,	r0,	1
	QBNE	DELAYON, r0, 0
	CLR		r30.t1
	CLR		r5.t00
	SBBO	r5,	r4,	0,	1
	MOV		r0,	r2
	
DELAYOFF:
	SUB		r0,	r0,	1
	QBNE	DELAYOFF, r0, 0	
	QBA		MAINLOOP

END:
	HALT