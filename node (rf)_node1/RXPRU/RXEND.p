
.setcallreg  r29.w2		 // set a non-default CALL/RET register
.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back
#define PREAMBLE_LEN 3
#define MTU_VALUE 100
#define ADDRESS_LENGTH 2
#define SEQ_NUMBER_LENGTH 4
#define ECC_LEN	16					// [TODO] Change to 2 when receiving packet from kernel is already implemented
#define OTHER_PACKET_CONTENT_BYTE (ADDRESS_LENGTH * 2 + SEQ_NUMBER_LENGTH + ECC_LEN)
#define BUFFER_LIMIT 6004

START:
	QBA		END
	//INITIALIZATION
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     // * load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // * clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // * store the modified r0 back at the load addr
	
	//SETUP GPI FOR BIT-SHIFT INPUT
	LBCO	r0, C4, 8, 4	// * load SYSCFG reg into r0 (use c4 const addr)
	LDI		r0.b0, 0xAA		// 0-1: 0x2 (01), 2: 0x0 (0), 3-7: 0x15 (10101)
	LDI		r0.b1, 0x2		// 8-12: 0x2 (01)
	SBCO	r0, C4, 8, 4	// * store the modified r0 back to the CFG register
	
	MOV		r0, 0x00000000	// r0: PRU0 Base Address | Low Value
	MOV		r1, 0x00010000	// r1: Shared RAM Address
	MOV		r2, 1			// r2: High Value
	MOV		r3, 0			// r3: 16 bit input value
	MOV		r4, 0			// r4: Program State (0 = During Preamble Detection, 1 = OOK Decoding First Bit, 2 = OOK Decoding SecondBit)
	MOV		r5, 0			// r5: 16 time counter, to do the looping for processing each and every bit
	MOV		r6, 0			// r6: N-times shift, to get currently processed bit from the 16 bit input
	MOV		r7, 0			// r7: Value of currently processed bit (HIGH or LOW)
	//Preamble Detection Variables
	MOV		r8, 0			// r8: cnt_symbol
	MOV		r9, 0			// r9: cnt_preamble
	MOV		r10, 0			// r10: prev_symbol
	MOV		r11, 16			// r11: number of bits (16)
	//OOK Decoding Variables
	MOV		r12, 0			// r12: first bit of Encoded Data
	MOV		r13, 0			// r13: temp variable of currently decoded bit (HIGH or LOW)
	
	//Packet Decoding variables
	MOV		r14, 0			// r14: Packet processing state (0 = SFD, 1 = length, 2 = body, 3 = final)
	MOV		r15, 7			// r15: First variable for each packet processing state ( SFD: cnt_symbol | Length: cnt_symbol, Body: cnt_symbol), initialize with value for SFD
	MOV		r16, 0			// r16: Second variable for each packet processing state ( SFD: recv_sfd | Length: payload_len, Body: total_byte_number), initialize with value for SFD
	MOV		r17, 0xA3		// r17: SFD value
	
	//Payload Length Detection variables
	MOV		r18, 0			// r18: encoded_len
	MOV		r19, 0			// r19: buffer_len
	
	//Packet writing to memory variables
	MOV		r20, 0			// r20: current_offset
	MOV		r21, 0			// r21: written_byte / rx_ch
	MOV		r22, MTU_VALUE	// r22: MTU VALUE
	MOV		r23, 0			// r23: cur_written_byte_num
	MOV		r24, 0
	MOV		r25, 0
	MOV		r26, 0
	MOV		r27, BUFFER_LIMIT
	MOV		r28, 0			// [DEBUG]r28:To check how many iteration has passed to a certain point


	MOV		r10, 0x00040004						// * Initialize the value of Read Idx and Write Idx
	SBBO	r10, r0, 0, 4
	
	//Preamble Detection Initialization
	MOV		r8, 1								// * cnt_symbol <- 1
	MOV		r9, 0								// * cnt_preamble <- 0
	MOV		r10, 0								// * prev_symbol <- 0
	
	
	
	//WAIT UNTIL START BIT IS SET
	WBS		r31.t29

READDATA:
	WBS		r31.t28								// * Wait until 16-bit shift flag is SET
	MOV		r3, r31.w0							// * r3 <- last 16 bit of r31 (*** From current checking, the last 16 bit is stored in this region with LSB ORDER. Please adjust accordingly if its incorrect)
	MOV		r5, 16								// * r5 <- 16, program will loop 16 times to process all bits individually
	//ADD		r28, r28, 1
	
GET_CURRENT_BIT:
	SUB		r6, r5, 1
	LSR		r7, r3, r6							// * r7 <- INPUT_16_bit >> N_shift_times
	AND		r7, r7, 0x1							// * r7 = r7 & 0x1 (to get only the first bit of r7 after shift)

CHECK_STATE:
	QBEQ	PREAMBLE_DETECTION, r4, 0			// * IF state is 0 THEN do the PREAMBLE DETECTION
	QBEQ	DECODE_OOK_FIRST, r4, 1				// * IF state is 1 THEN do the OOK DECODING for first bit
	QBEQ	DECODE_OOK_SECOND, r4, 2			// * IF state is 2 THEN do the OOK DECODING for secibd bit
	QBA		END									// * ELSE state is invalid, halt program 
	
AFTER_BIT_PROCESSED:
	SUB		r5, r5, 1							// * After a bit is processed, substract the counter by 1 
	QBEQ	READDATA, r5, 0						// * IF all 16 bits are processed THEN wait for another input from Shift Register
	QBA 	GET_CURRENT_BIT						// * ELSE check state again to process next bit
	
END:
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     					// End of program -- below are the "procedures"


PREAMBLE_DETECTION:
	QBBC 	CHECK_CURR_HIGH, r8.t0				// * IF count_symbol == even THEN check if current_symbol_is_high_and_previous_is_low
	QBGT 	ADD_SYMBOL, r7, r10					// * OR count_symbol == odd THEN check if current_symbol_is_low_and_previous_is_high
	QBA 	RESET_PREAMBLE						// * ELSE reset the preamble detection

CHECK_CURR_HIGH:
	QBGT 	ADD_SYMBOL, r10, r7					// * IF Current Symbol > Previous Symbol THEN Add current symbol to detected preamble
	QBA	 	RESET_PREAMBLE						// * ELSE reset the preamble detection
	
ADD_SYMBOL:
	ADD		r8, r8, 1							// * cnt_symbol++ (Increment number of currently detected preamble symbol)
	QBA		AFTER_ADD_SYMBOL
	
RESET_PREAMBLE:
	MOV		r8, 1								// * cnt_symbol <- 1
	MOV		r9, 0								// * cnt_premable <- 0
	MOV 	r10, r7								// * prev_symbol  <- current_value
	
AFTER_ADD_SYMBOL:
	MOV		r10, r7								// * prev_symbol <- curr_symbol (Set current symbol as previous symbol for next iteration)
	QBLE 	ADD_PREAMBLE, r8, 8					// * IF cnt_symbol == 8 THEN call add preamble count function
	QBA		AFTER_BIT_PROCESSED					// * ELSE move to after bit processed
	
ADD_PREAMBLE:
	MOV		r8, 0								// * cnt_symbol <- 0	(Reset the cnt_symbol value for the next preamble)
	ADD		r9, r9, 1							// * cnt_preamble++		(Increment the number of preamble)
	QBLE 	PREAMBLE_DONE, r9, PREAMBLE_LEN		// * IF cnt_preamble == Length of Preamble THEN preamble is spotted
	QBA		AFTER_BIT_PROCESSED					// * ELSE move to after bit processed
	
PREAMBLE_DONE:
												// *** Reset the first and second variable of SFD detection
	MOV		r15, 7								// * cnt_symbol <- 7 
	MOV		r16, 0								// * cur_read_sfd <- 0 [Reset the current read value of SFD]
	
	MOV		r4, 1								// * Set the state to 1 to process next bits with OOK DECODING
	MOV		r14, 0								// * packet_processing_state <- 0 [Packet processing state to SFD for the next packet]
	QBA		AFTER_BIT_PROCESSED					// * Move to after bit processed
	
DECODE_OOK_FIRST:
	MOV		r12, r7								// * first_bit_of_encoded_data <- current_symbol
	
	MOV		r4, 2								// * state <- 2 (Process Second bit for decoding)
	QBA		AFTER_BIT_PROCESSED
	
DECODE_OOK_SECOND:
	QBGT 	WRITE_HIGH, r12, r7					// * IF previous_value < current_value THEN decoded_value = HIGH
	QBGT 	WRITE_LOW, r7, r12					// * IF previous_value > current_value THEN decoded_value = LOW
	//IF the program reach this part, means the received value is incorrect, go back to preamble detection
	MOV		r8, 1								// * cnt_symbol <- 1
	MOV		r9, 0								// * cnt_premable <- 0
	MOV 	r10, r7								// * prev_symbol  <- current_value
	
	MOV		r4, 0								// * state <- PREAMBLE_DETECTION
	QBA 	AFTER_BIT_PROCESSED
	
WRITE_HIGH:
	MOV		r13, 1								// * cur_decoded_value <- 1 (HIGH)
	QBA	CHECK_PACKET_STATE
	
WRITE_LOW:
	MOV		r13, 0								// * cur_decoded_value <- 0 (LOW)
											
CHECK_PACKET_STATE:
	QBEQ	SFD, r14, 0							// * IF packet_proc_state == 0 THEN proceed to check SFD
	QBEQ	LENGTH, r14, 1						// * ELSE IF packet_proc_state == 1 THEN proceed to detect length
	QBEQ	BODY, r14, 2						// * ELSE IF packet_proc_state == 2 THEN proceed to construct body of packet
	QBEQ	FINAL, r14, 3						// * ELSE IF packet_proc_state == 3 THEN proceed to finalizing packet and send to upper layer
	QBA		END									// * This part should never reached, if happens, halt the program
	
SFD:
	LSR		r16, r17, r15						// * cur_sfd_value <- (sfd_value>>cnt_symbol)
	XOR		r16, r16, r13						// * sfd_check_value <- cur_sfd_value XOR cur_decoded_value
	QBBS	SFD_FAILED, r16.t0					// * IF sfd_check_value == TRUE THEN SFD is failed [If the xor result is set, it means the value is different]
	QBEQ	TO_LENGTH, r15, 0					// * IF cnt_symbol == 0 [already received 8 bits] THEN compare received 8 bits with SFD value
	SUB		r15, r15, 1							// * cnt_symbol--
	
	MOV		r4, 1								// * move state to OOK Detection first step
	QBA		AFTER_BIT_PROCESSED					// * finished processing bit

SFD_FAILED:
	MOV		r8, 1								// * cnt_symbol <- 1
	MOV		r9, 0								// * cnt_premable <- 0
	MOV 	r10, r7								// * prev_symbol  <- current_value
	
	MOV		r4, 0								// * state <- PREAMBLE_DETECTION
	QBA		AFTER_BIT_PROCESSED
	
TO_LENGTH:
	//SETUP VARIABLES FOR LENGTH DETECTION
	MOV		r15, 15								// * cnt_symbol <- 15
	MOV		r16, 0								// * payload_len <- 0
	
	MOV		r4, 1								// * move state to OOK Detection first step
	MOV		r14, 1								// * packet_proc_state <- 1 [proceed to detect length]
	QBA		AFTER_BIT_PROCESSED					// * finished processing bit
	
LENGTH:
	LSL		r13, r13, r15						// * cur_decoded_value <- (cur_decoded_value<<cnt_symbol)
	ADD		r16, r16, r13						// * payload_len += cur_decoded_value
	QBEQ	PROCESS_LENGTH, r15, 0				// * IF cnt_symbol == 0 [already received 16 bits] THEN process the value of payload length
	SUB		r15, r15, 1							// * cnt_symbol--
	
	MOV		r4, 1								// * move state to OOK Detection first step
	QBA		AFTER_BIT_PROCESSED					// * finished processing bit

PROCESS_LENGTH:
	//Validate the length value
	QBEQ	LENGTH_INVALID, r16, 0				// * IF payload_len <= 0 THEN length is invalid
	QBLT	LENGTH_INVALID, r16, r22			// * IF payload_len > mtu THEN length is invalid
	
	//Write the 2 bytes of length to memory
	LBBO	r20, r0, 0, 2
	
	SBBO	r16, r0, r20, 2						// * store payload_len value to memory
	ADD		r20, r20, 2
	QBGT	BODY_VAR_INITIALIZATION, r20, r27
	MOV		r20, 4	

BODY_VAR_INITIALIZATION:	
												// *** Variable Initialization for Packet Body Construction
	MOV		r15, 7								// * cnt_symbol <- 7
	MOV		r21, 0								// * rx_ch <- 0
	MOV		r23, 0								// * cur_written_byte_num <- 0
	ADD		r16, r16, OTHER_PACKET_CONTENT_BYTE	// * total_byte_packet = payload_len + (SELF_ID byte + DEST_ID byte + SEQ_NUMBER byte)	 
	MOV		r14, 2								// * packet_proc_state <- 1 [proceed to construction of packet body]
	
	MOV		r4, 1								// * move state to OOK Detection first step
	QBA		AFTER_BIT_PROCESSED

LENGTH_INVALID:
												// *** Reset variables for preamble detection
	MOV		r8, 1								// * cnt_symbol <- 1
	MOV		r9, 0								// * cnt_preamble <- 0
	MOV		r10, 0								// * prev_symbol <- 0
	
	MOV		r4, 0								// * move state to Preamble Detection for the next packet
	QBA		AFTER_BIT_PROCESSED
	
BODY:
	LSL		r13, r13, r15						// * cur_decoded_value <- (cur_decoded_value<<cnt_symbol)
	ADD		r21, r21, r13						// * rx_ch += cur_decoded_value
	QBEQ	PROCESS_PACKET_BYTE, r15, 0				// * IF cnt_symbol == 0 [already received 8 bits] THEN process the value of payload content
	SUB		r15, r15, 1							// * cnt_symbol--
	
	MOV		r4, 1								// * move state to OOK Detection first step
	QBA		AFTER_BIT_PROCESSED					// * finished processing bit
	
PROCESS_PACKET_BYTE:
	ADD		r23, r23, 1							// * cur_written_byte_num++
	//QBEQ	END, r23, 3
	SBBO	r21, r0, r20, 1						// * Store written_byte to memory with offset = current_offset
	ADD		r20, r20, 1
	
	QBGT	RESET_VAR_PACKET_BYTE, r20, r27
	MOV		r20, 4	
	
RESET_VAR_PACKET_BYTE:
	QBLE	FINAL, r23, r16						// * IF total_byte_packet <= cur_written_byte_num THEN packet is completed
	//Reset the variables
	MOV		r15, 7								// * cnt_symbol <- 7
	MOV		r21, 0								// * rx_ch <- 0
	
	MOV		r4, 1								// * move state to OOK Detection first step
	QBA		AFTER_BIT_PROCESSED					// * finished processing bit
	
FINAL:
	LBBO	r25, r0, 2, 2						// * read_idx <- read from memory
	QBEQ	FINAL, r25, r20						// * IF read_idx == current_offset THEN loop to FINAL
	SBBO	r20, r0, 0, 2						// * ELSE store current_offset to memory for current write_idx
	ADD		r26, r26, 1							// * [DEBUG] Add number of byte received
												// *** Reset variables for preamble detection
	MOV		r8, 1								// * cnt_symbol <- 1
	MOV		r9, 0								// * cnt_preamble <- 0
	MOV		r10, 0								// * prev_symbol <- 0
	
	MOV		r4, 0								// * move state to Preamble Detection for the next packet
	QBA		AFTER_BIT_PROCESSED					// * finished processing bit