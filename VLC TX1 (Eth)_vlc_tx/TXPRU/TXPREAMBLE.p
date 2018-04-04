
.setcallreg  r29.w2		 // set a non-default CALL/RET register
.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back
#define CONST_PRUCFG	     C4
#define PREAMBLE_LEN	3
#define SFD_SIZE		1
#define DATA_LENGTH_BYTE 2
#define PACKET_SIZE		20			//In byte
#define ADDR_LIMIT	(PACKET_SIZE * 100)
#define ADDRESS_SIZE 2
#define SEQ_NUMBER_SIZE 4

START:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4    		// load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        		// clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     		// store the modified r0 back at the load addr
	
	MOV		r1, 0x00000000	 		// * r1 = base address for data (offset 6 of RAM)
	
	//Setup OUTPUT SHIFT MODE
	LBCO 	r4, C4, 0x8, 4			// * r4 = GPCFG0 for PRU0
	LDI		r4.b1, 0xC0
	LDI		r4.b2, 0x2A
	CLR		r4.t24
	SBCO 	r4, C4, 0x8, 4
	
	MOV		r5, 0					// * r5 = Temp variable to store last LOAD_GPO_SH state
	MOV		r6, 0					// * r6 = State of the Program
	MOV		r7,	0					// * r7 = Temp variable to store value for GPO SH
	CLR		r30.t31				
	
	//Load Initial Data in GP_SH0 with 0 bits
	SET 	r30.t29
	LDI		r30.b1, 0xAA
	LDI		r30.b0, 0xAA
	CLR		r30.t29

	//Load Initial Data in GP_SH1 with one 1 bit in the end to notify Receiver of input
	SET 	r30.t30
	LDI		r30.b1, 0xAA
	LDI		r30.b0, 0xAA
	CLR 	r30.t30
	
	SET 	r30.t31					// - Set PRU_ENABLE_SHIFT bit to Start Shift Operation
	
SHIFTLOOP:
	LBCO 	r4, C4, 0x8, 4			// - Read LOAD_GPO_SH from CFG
	QBBC	POLL0, r5.t0			// - If last LOAD_GPO_SH is 0, wait until it changes to 1
	QBBS	SHIFTLOOP, r4.t25		// - Else if last LOAD_GPO_SH == 1, wait until current LOAD_GPO_SH value changes to 0 
	MOV		r5, 0					// - If current LOAD_GPO_SH changed to 0, set the last LOAD_GPO_SH to 0
	QBA		SETTIMINGFLAG				// - Load the data to GPO SH 1
	
POLL0:								//Wait until LOAD_GPO_SH change to 1
	QBBC	SHIFTLOOP, r4.t25		// - If last LOAD_GPO_SH == 0, wait until current LOAD_GPO_SH value changes to 1 
	MOV		r5, 1					// - set the last LOAD_GPO_SH to 1
	QBA		CLRTIMINGFLAG				// - Load the data to GPO SH 0

SETTIMINGFLAG:
	SET		r30.t5
	LBCO	r0, C4, 8, 4	// * load SYSCFG reg into r0 (use c4 const addr)
	SET		r12.t13
	SBCO	r0, C4, 8, 4
	QBA		SHIFTLOOP
	
CLRTIMINGFLAG:
	CLR		r30.t5
	LBCO	r0, C4, 8, 4	// * load SYSCFG reg into r0 (use c4 const addr)
	SET		r12.t13
	SBCO	r0, C4, 8, 4
	QBA		SHIFTLOOP
	
END:
	//CLR	r30.t31
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"