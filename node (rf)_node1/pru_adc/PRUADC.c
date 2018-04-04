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
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define ADC_PRU_NUM	   1   // using PRU0 for the ADC capture
#define OOK_PRU_NUM	   0   // using PRU1 for the sample clock
#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define MMAP1_LOC   "/sys/class/uio/uio0/maps/map1/"

#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)

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

enum CONTROL {
	PAUSED = 0,
	RUNNING = 1,
	UPDATE = 3
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

void *execute_pru(void *tid){
	// Load and execute the PRU program on the PRU
   prussdrv_exec_program (ADC_PRU_NUM, "./PRUADC.bin");
   prussdrv_exec_program (OOK_PRU_NUM, "./PRUOOK.bin");
//   printf("EBBClock PRU1 program now running (%d)\n", ookData[0]);

   // Wait for event completion from PRU, returns the PRU_EVTOUT_0 number
   int n = prussdrv_pru_wait_event (PRU_EVTOUT_0);
   printf("EBBADC PRU0 program completed, event number %d.\n", n);

   pthread_exit(NULL);
}

void *read_value(void *tid){
	int fd;
	void *map_base, *virt_addr;
	unsigned long read_result, writeval;
	unsigned int addr = readFileValue(MMAP1_LOC "addr");
	unsigned int dataSize = readFileValue(MMAP1_LOC "size");
	unsigned int numberOutputSamples = dataSize * 2;
	off_t target = addr;

	int current_n_sample = 0;
	int current_reading = 0;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
		printf("Failed to open memory!");
		exit(-1);
	}
	fflush(stdout);

	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) {
	   printf("Failed to map base address");
	   exit(-1);
	}
	fflush(stdout);
//
	for(;;){
		target = addr;
		virt_addr = map_base + (target & MAP_MASK);
		read_result = *((unsigned long *) virt_addr);
//		printf("Value at address 0x%X (%p): 0x%X\n", target, virt_addr, read_result);
		printf("current index: %d\n",(int)read_result);
//		if((int)read_result != current_n_sample){
//			current_n_sample = (int)read_result;
//
//			target = addr + 4 + (byte_capacity - current_n_sample);
//			virt_addr = map_base + (target & MAP_MASK);
//			read_result = *((uint16_t *) virt_addr);
//			current_reading = (int)read_result;
//			printf("current reading: %d\n",current_reading);
//			printf("-----------\n");
//		}

	}

	pthread_exit(NULL);
}

int main (void)
{
   int i;
   if(getuid()!=0){
      printf("You must run this program as root. Exiting.\n");
      exit(EXIT_FAILURE);
   }
   // Initialize structure used by prussdrv_pruintc_intc
   // PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
   tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

   // Read in the location and address of the shared memory. This value changes
   // each time a new block of memory is allocated.
   unsigned int ookData[2];
   ookData[0] = readFileValue(MMAP1_LOC "addr");
   ookData[1] = 0x1111 - (unsigned int) 6;
//   printf("The PRU clock state is set as period: %d (0x%x) and state: %d\n", timerData[0], timerData[0], timerData[1]);
//   unsigned int PRU_data_addr = readFileValue(MMAP0_LOC "addr");
//   printf("-> the PRUClock memory is mapped at the base address: %x\n", (PRU_data_addr + 0x2000));
//   printf("-> the PRUClock on/off state is mapped at address: %x\n", (PRU_data_addr + 0x10000));

   // data for PRU0 based on the MCPXXXX datasheet
   unsigned int spiData[3];
   spiData[0] = 0x00000000;
   spiData[1] = FREQ_1MHz;
//   spiData[2] = readFileValue(MMAP1_LOC "size");
   spiData[2] = 0x1111 - (unsigned int) 6;
//   printf("Sending the SPI Control Data: 0x%x\n", spiData[0]);
//   printf("The DDR External Memory pool has location: 0x%x and size: 0x%x bytes\n", spiData[1], spiData[2]);
//   int numberSamples = spiData[2]/2;
//   printf("-> this space has capacity to store %d 16-bit samples (max)\n", numberSamples);

   // Allocate and initialize memory
   prussdrv_init ();
   prussdrv_open (PRU_EVTOUT_0);
   // Write the address and size into PRU0 Data RAM0. You can edit the value to
   // PRUSS0_PRU1_DATARAM if you wish to write to PRU1
   prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, spiData, 12);  // spi code
   prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, ookData, 8); // sample clock
   // Map the PRU's interrupts
   prussdrv_pruintc_init(&pruss_intc_initdata);

   pthread_t threads[2];
   if(pthread_create(&threads[0],NULL,execute_pru,(void *)0)
		   ||pthread_create(&threads[1],NULL,read_value,(void *)1)){
	   printf("error initiating thread\n");
	   exit(-1);
   }
   for(i=0; i<2; i++) {
	   pthread_join(threads[i], NULL);
   }
// Disable PRU and close memory mappings
   prussdrv_pru_disable(ADC_PRU_NUM);
   prussdrv_pru_disable(OOK_PRU_NUM);

   prussdrv_exit ();
   return EXIT_SUCCESS;
}
