// PRU program to communicate to the ADS7883 family of SPI ADC ICs. To use this 
// program as is, use the following wiring configuration:
//   Chip Select (CS):   P9_27    pr1_pru0_pru_r30_5  r30.t5
//   MISO            :   P9_28    pr1_pru0_pru_r31_3  r31.t3
//   CLK             :   P9_30    pr1_pru0_pru_r30_2  r30.t2
//   Sample Clock    :   P8_46    pr1_pru1_pru_r30_1  -- for testing only
// This program was writen by Derek Molloy to align with the content of the book 
// Exploring BeagleBone. See exploringbeaglebone.com/chapter13/

.setcallreg  r29.w2		 			// set a non-default CALL/RET register
.origin 0                        	// start of program in PRU memory
.entrypoint START                	// program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    	// allows notification of program completion
#define PRU_EVTOUT_0    3        	// the event number that is sent back

START:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     	// load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        	// clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     	// store the modified r0 back at the load addr

	MOV	r1, 0x00000000	 		// load the base address into r1
				 				// PRU memory 0x00 stores the SPI command - e.g., 0x01 0x80 0x00
				 				// the SGL/DIFF and D2, D1, D0 are the four LSBs of byte 1 - e.g. 0x80
	MOV	r7, 0x00000FFF	 		// the bit mask to use on the returned data (i.e., keep 10 LSBs only)
	LBBO    r8, r1, 4, 4     	// * r8 = memory address to store sample values
	MOV		r9, 1000			// * r9 = threshold value for HIGH
	//MOV		r9, 1
	MOV		r6, 4				// * r6 = current OFFSET for writing in memory
	//LBBO	r10, r1, 12, 4		// * r10 = delay of the clock
	MOV		r10, 1
	MOV		r5, 0x00010000		// * r5 = shared RAM address

	MOV	r3, 0x00000000	 		// clear r3 to receive the response from the MCP3XXX
	CLR	r30.t2		 			// set the clock low

GET_SAMPLE:
	// Need to wait at this point until it is ready to take a sample - i.e., 0x00010000 
	// store the address in r5
	MOV r11, r10 				// r11 = variables as counter for clock
	//ADD	r11, r11, 1 			// balance duty cycle by looping 1 extra time on low
	
SAMPLE_WAIT_HIGH:		 		// wait until the PRU1 sample clock goes high
	SUB	r11, r11, 1		
	QBNE	SAMPLE_WAIT_HIGH, r11, 0 	// wait until the sample clock goes high
	MOV	r11, r10 						//Reset the clock counter
	CLR	r30.t5		 					// set the CS line low (active low)
	MOV	r4, 16		 					// going to write/read 16 bits (2 bytes)
	
SPICLK_BIT:                      		// loop for each of the 16 bits
	SUB	r4, r4, 1        				// count down through the bits
// *************************************
	LSL	r3, r3, 1        				// shift the captured data left by one position 
	
	// Clock goes low for a time period
	SET	r30.t2		 					// set the clock high
	SET	r30.t2		 					// set the clock high
	SET	r30.t2		 					// set the clock high
	QBBC	ORZERO, r31.t3   		// check if the bit that is read in is low? jump
	OR	r3, r3, 0x00000001  			// set the stored bit LSB to 1 otherwise
	QBA	DATAINLOW
	
ORZERO:
	OR	r3, r3, 0x00000000  			// clear the stored bit LSB otherwise
		
DATAINLOW:	                 
	CLR	r30.t2		 					// set the clock low
	CLR	r30.t2		 					// set the clock low
// **************************************
	QBNE	SPICLK_BIT, r4, 0  			// have we performed 16 cycles?

	LSR	r3, r3, 2        				// SPICLK shifts left too many times left, shift right once
	AND	r3, r3, r7	 					// AND the data with mask to give only the 10 LSBs
	SET	r30.t5		 					// pull the CS line high (end of sample)

STORE_DATA:                      		// store the sample value in memory
	//XOR		r30.b0, r30.b0, 1<<1
	QBEQ	STOREHIGH, r9, 1
	CLR		r30.t1
	MOV		r9, 1
	//QBLT	STOREHIGH, r3.w0, r9			// If the reading value of ADC is more than threshold, output HIGH
	//CLR		r30.t1
	//SBBO	r1, r5, 0, 1
	QBA		SAMPLE_WAIT_LOW
	
STOREHIGH:
	SET		r30.t1
	MOV		r9, 0
	//SBBO	r10, r5, 0, 1

SAMPLE_WAIT_LOW:                 		// need to wait here if the sample clock has not gone low
	SUB	r11, r11, 1
	QBNE	SAMPLE_WAIT_LOW, r11, 0 	// wait until the sample clock goes low (just in case)
	QBA	GET_SAMPLE

END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"