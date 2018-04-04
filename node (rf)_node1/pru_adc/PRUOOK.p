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

START:	                         // aims to load the clock period value into r2
	MOV	r1, 0x00000000	 // load the base address into r1
	LBBO	r2, r1, 0, 4	 // the global memory address to store result
	MOV	r4, 0x00010000   // Shared Memory base address IDX 0-3: current n byte (r4), 4-~: actual data (r5)
	LBBO	r7, r1, 4, 4 //Max capacity of  shared memory
	LBBO r3,r4,0,4 //Initial value of current index

MAINLOOP:
	LBBO r6,r4,0,4
	QBEQ MAINLOOP,r6,r3 //If the current index is still the same, wait
	
DATAREADING: //Current index in PRU1 is moved
	LBBO r8, r4, r3, 2
	SBBO r8, r2, r3, 2
	
	ADD r3,r3,2
	QBNE CMPIDX, r3, r7
	MOV r3, 4

CMPIDX:
	QBNE DATAREADING, r3, r6 
	QBA MAINLOOP

END:
	HALT