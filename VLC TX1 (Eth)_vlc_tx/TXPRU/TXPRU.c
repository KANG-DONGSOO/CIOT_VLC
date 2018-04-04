/*  This program loads the two PRU programs into the PRU-ICSS transfers the configuration
*   to the PRU memory spaces and starts the execution of both PRU programs.
*   pressed. By Derek Molloy, for the book Exploring BeagleBone. Please see:
*        www.exploringbeaglebone.com/chapter13
*   for a full description of this code example and the associated programs.
*/

#include <stdio.h>
#include <stdlib.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#define TX_PRU_NUM	   0   // using PRU0 for the ADC capture
#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define MMAP1_LOC   "/sys/class/uio/uio0/maps/map1/"

enum FREQUENCY {    // measured and calibrated, but can be calculated
	FREQ_12_5MHz =  1,
	FREQ_6_25MHz =  5,
	FREQ_5MHz    =  7,
	FREQ_3_85MHz = 10,
	FREQ_1MHz   =  45,
	FREQ_500kHz =  95,
	FREQ_250kHz = 245,
	FREQ_100kHz = 495,
	FREQ_25kHz = 1995,
	FREQ_10kHz = 4995,
	FREQ_5kHz =  9995,
	FREQ_2kHz = 24995,
	FREQ_1kHz = 49995
};

// Short function to load a single unsigned int from a sysfs entry
unsigned int readFileValue(char filename[]){
   FILE* fp;
   unsigned int value = 0;
   fp = fopen(filename, "rt");
   fscanf(fp, "%x", &value);
   fclose(fp);
   return value;
}

int main (void)
{
   if(getuid()!=0){
      printf("You must run this program as root. Exiting.\n");
      exit(EXIT_FAILURE);
   }
   // Initialize structure used by prussdrv_pruintc_intc
   // PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
   tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

   // Read in the location and address of the shared memory. This value changes
   // each time a new block of memory is allocated.
   unsigned int max_offset_value = 3996;

   unsigned int tx_data[2];
   tx_data[0] = readFileValue(MMAP0_LOC "addr");
   tx_data[1] = readFileValue(MMAP0_LOC "size");

   // Allocate and initialize memory
   prussdrv_init ();
   prussdrv_open (PRU_EVTOUT_0);

   // Write the address and size into PRU0 Data RAM0. You can edit the value to
   // PRUSS0_PRU1_DATARAM if you wish to write to PRU1
   //prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, tx_data, 8);  // spi code

   // Map the PRU's interrupts
   prussdrv_pruintc_init(&pruss_intc_initdata);

   // Load and execute the PRU program on the PRU
   prussdrv_exec_program (TX_PRU_NUM, "./TXPRU.bin");
  // prussdrv_exec_program (1, "./PRUPREAMBLE.bin");
//   printf("EBBClock PRU1 program now running (%d)\n", timerData[0]);

   // Wait for event completion from PRU, returns the PRU_EVTOUT_0 number
   int n = prussdrv_pru_wait_event (PRU_EVTOUT_0);
   printf("EBBDAC PRU0 program completed, event number %d.\n", n);

// Disable PRU and close memory mappings
   prussdrv_pru_disable(TX_PRU_NUM);

   prussdrv_exit ();
   return EXIT_SUCCESS;
}
