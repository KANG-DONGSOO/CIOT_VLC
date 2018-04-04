
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
#define ADDRESS_LENGTH 2
#define SEQ_NUMBER_LENGTH 4
#define OTHER_PACKET_CONTENT_BYTE (ADDRESS_LENGTH * 2 + SEQ_NUMBER_LENGTH)

START:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4    		// * load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        		// * clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     		// * store the modified r0 back at the load addr
	
	MOV		r0, 0x00000000	 		// * r0 = base address for data (offset 6 of RAM)
	MOV		r4, 0
	
	LBBO	r1, r0, 0, 4
	LBBO	r2, r0, 4, 4
	LBBO	r3, r0, 8, 4
	LBBO	r4, r0, 12, 4
	
	
END:
	CLR	r30.t31
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"