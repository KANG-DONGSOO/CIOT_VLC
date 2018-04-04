
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
#define ECC_LEN	2					// [TODO] Change to 2 when receiving packet from kernel is already implemented
#define OTHER_PACKET_CONTENT_BYTE (ADDRESS_LENGTH * 2 + SEQ_NUMBER_LENGTH + ECC_LEN)

START:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4    		// * load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        		// * clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     		// * store the modified r0 back at the load addr
	
	//Setup OUTPUT SHIFT MODE
	LBCO 	r0, C4, 0x8, 4			// * r0 = GPCFG0 for PRU0
	LDI		r0.b1, 0xC0
	LDI		r0.b2, 0x2A
	CLR		r0.t24
	SBCO 	r0, C4, 0x8, 4
	
	MOV		r0, 0x00000000	 		// * r0 = base address for data (offset 6 of RAM)
	
	//DUMMY VALUE FOR DATA STORED IN MEMORY
	//MOV		r8, 0x00010004
	//SBBO	r8, r0, 0, 4
	//MOV		r8, 0xEF120008
	//SBBO	r8, r0, 4, 4
	//MOV		r8, 0xCACAABCD
	//SBBO	r8, r0, 8, 4
	//MOV		r8, 0xE95ECACA
	//SBBO	r8, r0, 12, 4
	
	MOV		r1, 0					// * r1 = transmitted_bit_counter	[variable used to count how much transmitted bit are already put in GETVALUE procedure]
	MOV		r2, 0					// * r2 = state of the program (0 = Preamble Detection, 1 = SFD transmission, 2 = length encoding, 3 = payload encoding)
	MOV		r3, 0					// * r3 = Temp variable to store value for GPO SH
	MOV		r4, 0					// * r4 = temp variable for GPCFG value
	MOV		r5, 0					// * r5 = last_GPO_used [variable to check which GPO was used last to determine which one should be polled next]
	MOV		r6, 0					// * r6 = State of the Program
	MOV		r7, 7					// * r7 = 1st parameter of each state (Preamble = cnt_symbol | SFD = cnt_symbol | Body: cnt_symbol)
	MOV		r8, 0xA3				// * r8 = SFD value
	MOV		r9, 0					// * r9 = curr_offset [Offset value to read from memory]
	MOV		r10, 0					// * r10 = packet_length_value
	MOV		r11, 0					// * r11 = Preamble = cur_preamble_bit [to store current preamble bit] | others: curr_encoded_bit [to store currently encoded bit]
	MOV		r12, 0					// * r12 = total_packet_bytes [Payload + address and sequence number]
	MOV		r13, 2					// * r13 = reversed OOK Encoded value of HIGH (HIGH - LOW) 
	MOV		r14, 1					// * r14 = reversed OOK Encoded value of LOW (LOW - HIGH)
	MOV		r15, 0					// * r15 = encoded_byte_counter	[Counter for the number of encoded byte from the packet]
	MOV		r16, 0					// * r16 = current_encoded_packet_byte	[To store the currently encoded byte value]
	MOV		r17, 0xAA				// * r17 = Preamble value
	MOV		r18, 0					// * r18 = cnt_preamble
	
	
	MOV		r19, 0					// [DEBUG] * r19: to count iteration of GETVALUE called, checking content of encoded bit one for each execution of GETVALUE
	MOV		r21, 0
	MOV		r22, 0					// * r22 = temp_encoded_bit_value
	MOV		r23, ADDR_LIMIT
	
	CLR		r30.t31				

	//Load Initial Data in GP_SH0 with 0 bits
	SET 	r30.t29
	MOV		r30.b1, r3.b1
	MOV		r30.b0, r3.b0
	CLR		r30.t29
	
	//Load Initial Data in GP_SH1 with one 1 bit in the end to notify Receiver of input
	SET 	r30.t30
	MOV		r30.b1, r3.b1
	MOV		r30.b0, r3.b0
	CLR 	r30.t30
	
	SET 	r30.t31					// - Set PRU_ENABLE_SHIFT bit to Start Shift Operation
	
SHIFTLOOP:
	LBCO 	r4, C4, 0x8, 4			// - Read LOAD_GPO_SH from CFG
	QBBC	POLL0, r5.t0			// - If last LOAD_GPO_SH is 0, wait until it changes to 1
	QBBS	SHIFTLOOP, r4.t25		// - Else if last LOAD_GPO_SH == 1, wait until current LOAD_GPO_SH value changes to 0 
	MOV		r5, 0					// - If current LOAD_GPO_SH changed to 0, set the last LOAD_GPO_SH to 0
	QBA		LOADGPOSH1				// - Load the data to GPO SH 1
	
POLL0:								//Wait until LOAD_GPO_SH change to 1
	QBBC	SHIFTLOOP, r4.t25		// - If last LOAD_GPO_SH == 0, wait until current LOAD_GPO_SH value changes to 1 
	MOV		r5, 1					// - set the last LOAD_GPO_SH to 1
	QBA		LOADGPOSH0				// - Load the data to GPO SH 0
	
LOADGPOSH0:
	CALL	GETVALUE				// - Call procedure to get next transmitted value
	SET 	r30.t29					// - Set the flag to load GPO SH0
	MOV		r30.b1, r3.b1			// - GPO SH0.b1 <- current_value.b1
	MOV		r30.b0, r3.b0			// - GPO SH0.b0 <- current_value.b0
	CLR		r30.t29					// - Clear the flag to load GPO SH0
	CLR		r30.t5					// [DEBUG] Check what data is sent during this period (Load GPO SH0/Output data from GPO SH1)
	QBA		SHIFTLOOP				// - Return to Shift Loop

LOADGPOSH1:
	CALL	GETVALUE				// - Call procedure to get currently transmitted value
	SET 	r30.t30					// - Set the flag to load GPO SH1
	MOV		r30.b1, r3.b1			// - GPO SH1.b1 <- current_value.b1
	MOV		r30.b0, r3.b0			// - GPO SH1.b0 <- current_value.b0
	CLR 	r30.t30					// - Clear the flag to load GPO SH1
	SET		r30.t5					// [DEBUG] Check what data is sent during this period (Load GPO SH1/Output data from GPO SH0)
	QBA		SHIFTLOOP
	
END:
	CLR	r30.t31
	MOV	r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0	
	HALT                     // End of program -- below are the "procedures"


//FUNCTION GETVALUE: DETERMINE THE NEXT 16 BITS TO BE ENCODED BY PRU
//
//

GETVALUE:
	//ADD		r19, r19, 1						// [DEBUG] used to execute GETVALUE function with desired number of executions [Validating result of each step of encoding]
	MOV		r1, 0							// - transmitted_bit_counter <- 0
	MOV		r3,	0 							// - reset current_value to zero
	
CHECK_STATE:
	QBEQ	PREAMBLE, r2, 0					// - IF curr_state == 0 THEN Set the GPO SH data to preamble
	QBEQ	SFD, r2, 1						// - IF curr_state == 1 THEN set the GPO SH data to SFD
	QBEQ	DATALENGTH, r2, 2				// - IF curr_state == 2 THEN set the GPO SH data to Payload Length
	QBEQ	DATAOOK, r2, 3					// - IF curr_state == 3 THEN set the GPO SH data to Packet content
	QBEQ	PACKET_END, r2, 4				// -IF curr_state == 4 THEN set the GPO SH data to Packet Separator
	QBA		END								// - ELSE program should not reach this section, if it does, halt the program and there is something wrong
	
PREAMBLE:
	LSR		r11, r17, r7					// - curr_preamble_bit = (preamble_value>>cnt_symbol)
	AND		r11, r11, 0x1					// - curr_preamble_bit = curr_preamble_bit & 1 [Get last bit of current preamble]
	LSL		r11, r11, r1					// - curr_preamble_bit = curr_preamble_bit<<transmitted_bit_counter 
	ADD		r3, r3, r11						// - current_value+= curr_preamble_bit
	ADD		r1, r1, 1						// - transmitted_bit_counter++
	QBEQ	ADD_PREAMBLE, r7, 0				// - IF cnt_symbol == 0 THEN increase cnt_preamble [One byte of preamble is sent]
	SUB		r7, r7, 1						// - cnt_symbol--
	QBA		AFTERGETVALUE

ADD_PREAMBLE:
	ADD		r18, r18, 1						// - cnt_preamble++
	QBEQ	TO_SFD, r18, PREAMBLE_LEN
	//Reset the variables for preamble detection
	MOV		r7, 7
	QBA		AFTERGETVALUE

//TO_PREAMBLE: //[DEBUG] If we only want to output PREAMBLE, jump to this section instead of TO_SFD
//	MOV		r2, 0							// - curr_state = 0 (PREAMBLE)
	
	//Initialize variables for next preamble detection
//	MOV		r7, 7							// - cnt_symbol = 7
//	MOV		r18, 0							// - cnt_preamble = 0
//	QBA		AFTERGETVALUE
	
TO_SFD:
	MOV		r2, 1							// - curr_state <- 1 (SFD)
	
	//Set variables for SFD
	MOV		r7, 7							// - cnt_symbol <- 7
	
	QBA		AFTERGETVALUE
	
SFD:
	LSR		r11, r8, r7						// - currently_encoded_bit = (sfd_value>>cnt_symbol)
	QBEQ	TO_LENGTH, r7, 0				// - IF cnt_symbol == 0 THEN change state to Payload length encoding
	SUB		r7, r7, 1						// - cnt_symbol--

ENCODEBIT:
	MOV		r22, 0							// - temp_encoded_bit_value = 0 [Reset the temp variable]
	QBBS	ENCODEHIGH, r11.t0
	QBA		ENCODELOW

TO_LENGTH:
	//[DEBUG]  Skip to Packet End after SFD in order to check only Preamble and SFD
	//MOV		r2, 4							// - curr_state = 0 (PACKET_END)
	
	//Initialize variables for next preamble detection
	//MOV		r7, 3							// - cnt_symbol = 7
	//MOV		r18, 0							// - cnt_preamble = 0
	//end [DEBUG]
	
	//Change the state to data length
	MOV		r2, 2							// - curr_state <- 2 (DATALENGTH)
	
	//Initialize variables for Payload Length Encoding
	MOV		r7, 15							// - cnt_symbol = 15 [The size of length is two byte ~ 16 bits]
	MOV		r9, 0
	ADD		r21, r21, 1
	LBBO	r10, r0, r9, 2					// - packet_length_value = value read from memory (first 2 bytes of the packet)
	ADD		r9, r9, 2						// - current_offset += 2 [Increase offset by 2 since the last 2 bytes are packet length]
	QBA		ENCODEBIT						// - encode current SFD bit

ENCODEHIGH:
	LSL		r22, r13, r1					// - temp_encoded_bit_value = HIGH_VALUE<<transmitted_bit_counter
	ADD		r3, r3, r22						// - current_value += temp_encoded_bit_value
	ADD		r1, r1, 2						// - transmitted_bit_counter += 2 [2 bits of encoded value]
	QBA		AFTERGETVALUE
	
ENCODELOW:
	LSL		r22, r14, r1					// - temp_encoded_bit_value = LOW_VALUE<<transmitted_bit_counter
	ADD		r3, r3, r22						// - current_value += temp_encoded_bit_value
	ADD		r1, r1, 2						// - transmitted_bit_counter += 2 [2 bits of encoded value]
	QBA		AFTERGETVALUE

DATALENGTH:
	LSR		r11, r10, r7					// - currently_encoded_bit = (packet_length_value>>cnt_symbol)
	QBEQ	TO_BODY, r7, 0					// - IF cnt_symbol == 0 THEN packet length encoding complete, move to packet body
	SUB		r7, r7, 1
	QBA		ENCODEBIT						// - encode current data length bit
	
TO_BODY:
	//[DEBUG] Skip to Packet End after Length in order to check only up to length
	//MOV		r2, 4							// - curr_state = 0 (PACKET_END)
	
	//Initialize variables for next preamble detection
	//MOV		r7, 3							// - cnt_symbol = 7
	//MOV		r18, 0							// - cnt_preamble = 0
	//end [DEBUG]
	
	//Change the state to the next one
	MOV		r2, 3									// - curr_state = 3 (DATAOOK)
	
	//Initialize variables for Packet Content Encoding
	MOV		r7, 7									// - cnt_symbol = 7 [Reset count symbol to 7, to read each byte of the packet
	MOV		r15, 0									// - encoded_byte_counter = 0 [In the beginning, no byte has been encoded, so reset this variable]
	ADD		r12, r10, OTHER_PACKET_CONTENT_BYTE		// - total_packet_bytes = packet_length_value + (ADDRESS_LENGTH * 2 + SEQ_NUMBER_LENGTH)
	//MOV		r12, 4									// [DEBUG] Modify the number of packets to be transmitted to check the correctness of transmitted value
	LBBO	r16, r0, r9, 1							// current_encoded_packet_byte = read from memory
	ADD		r9, r9, 1								// curr_offset++
	
	QBA		ENCODEBIT								// - encode current data length bit
	
DATAOOK:
	LSR		r11, r16, r7					// - currently_encoded_bit = (current_encoded_packet_byte>>cnt_symbol)
	QBEQ	NEXT_BYTE, r7, 0				// - IF cnt_symbol == 0 THEN load next byte [since all bits from current byte is already encoded]
	SUB		r7, r7, 1						// - cnt_symbol--
	QBA		ENCODEBIT

NEXT_BYTE:
	//Reset variables for Packet Content Encoding
	MOV		r7, 7							// - cnt_symbol = 7
	ADD		r15, r15, 1						// - encoded_byte_counter++ [Number of byte that are encoded increased]
	QBEQ	PACKET_DONE, r15, r12			// - IF encoded_byte_counter == total_packet_bytes THEN current packet is already encoded
	LBBO	r16, r0, r9, 1							// current_encoded_packet_byte = read from memory
	ADD		r9, r9, 1								// curr_offset++
	QBA		ENCODEBIT

PACKET_DONE:
	//After Packet is done, move to the Packet End Section
	MOV		r2, 4							// - curr_state = 0 (PACKET_END)
	
	//Initialize variables for next preamble detection
	MOV		r7, 3							// - cnt_symbol = 7
	MOV		r18, 0							// - cnt_preamble = 0
	QBA		ENCODEBIT
	
PACKET_END:
	LSL		r18, r14, r1
	ADD		r3, r3, r18
	ADD		r1, r1, 1
	QBEQ	BACK_TO_PREAMBLE, r7, 0
	SUB		r7, r7, 1
	QBA		AFTERGETVALUE
	
BACK_TO_PREAMBLE:
	MOV		r2, 0							// - curr_state = 0 (PREAMBLE)
	
	//Initialize variables for next preamble detection
	MOV		r7, 7							// - cnt_symbol = 7
	MOV		r18, 0							// - cnt_preamble = 0
	
	QBA		AFTERGETVALUE
			
AFTERGETVALUE:								
	QBNE	CHECK_STATE, r1, 16				// - IF transmitted_bit_counter < 16 THEN check state again to get more bits
	//QBEQ	END, r19, 10						// [DEBUG] used to execute GETVALUE function with desired number of executions [Validating result of each step of encoding]
	RET