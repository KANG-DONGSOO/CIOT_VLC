#include <stdio.h>
#include <stdlib.h>
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
#include <sys/io.h>

#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)
#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define PACKET_SIZE 20
#define ADDR_LIMIT (PACKET_SIZE * 100)

unsigned int readFileValue(char filename[]){
   FILE* fp;
   unsigned int value = 0;
   fp = fopen(filename, "rt");
   fscanf(fp, "%x", &value);
   fclose(fp);
   return value;
}

int main(int argc, char **argv) {
    int fd,i;
    unsigned long read_pointer, write_pointer;
	void *map_base, *write_pointer_addr, *read_pointer_addr, *data_pointer_addr, *curr_pointer;
    unsigned long read_result = 0;
    uint8_t read_result1 = 0, read_result2 = 0,curr_pkt_data;
    unsigned int addr = readFileValue(MMAP0_LOC "addr");
    unsigned int dataSize = readFileValue(MMAP0_LOC "size");
    off_t target = addr;

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
	printf("Failed to open memory!");
	return -1;
    }
    fflush(stdout);

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
       printf("Failed to map base address");
       return -1;
    }
    fflush(stdout);


    read_pointer_addr = map_base + (target+2 & MAP_MASK);
	write_pointer_addr = map_base + (target+4 & MAP_MASK);
	data_pointer_addr = map_base + (target+6+20 & MAP_MASK);

//	*((uint16_t *) read_pointer_addr) = (uint16_t) 40;

	read_pointer = *((uint16_t *) data_pointer_addr);
	printf("Data: %d\n",read_pointer);

//////	printf("size of int: %d\n",sizeof(int));
//    for(;;){
////    	do{
//    		read_pointer = *((uint16_t *) read_pointer_addr);
//			write_pointer = *((uint16_t *) write_pointer_addr);
////			usleep(1000);
////    	}
////    	while(read_pointer == write_pointer);
//
//    	printf("Read Pointer: %d\n",read_pointer);
//		printf("Write Pointer: %d\n",write_pointer);
//
////		curr_pointer = map_base + (target+6+read_pointer & MAP_MASK);
//////
//////		for(i=0;i<6;i+=2){
//////			read_result = *((unsigned short *) curr_pointer+i);
//////			printf("0x%02x ",read_result);
//////		}
//////		printf("\n");
////
////		read_result1 =  *((uint8_t *) curr_pointer);
////		read_result2 =  *((uint8_t *) curr_pointer+1);
////		read_result = (read_result2<<8) | read_result1;
////		printf("Length of packet: %d\n",read_result1, read_result2);
////
////		read_result1 = *((uint8_t *) curr_pointer+2);
////		read_result2 = *((uint8_t *) curr_pointer+3);
////		read_result = (read_result2<<8) | read_result1;
////    	printf("Self ID: %d\n",read_result);
////
////    	read_result1 = *((uint8_t *) curr_pointer+4);
////		read_result2 = *((uint8_t *) curr_pointer+5);
////		read_result = (read_result2<<8) | read_result1;
////		printf("Dest ID: %d\n",read_result);
////
////		unsigned int seq_num = 0;
////		for(i=0;i<4;i++){
////			uint8_t temp = *((uint8_t *) curr_pointer+6+i);
//////			printf("0x%04x ",temp);
////			seq_num = seq_num | (temp<<(i*8));
////		}
//////		printf("\n");
//////		seq_num = (unsigned int) *((uint8_t *) curr_pointer+6);
////		printf("Seq Number: %d\n",seq_num);
////
////		printf("Data:\n");
////    	for(i=0;i<10;i++){
////    		curr_pkt_data = *((uint8_t *) curr_pointer + 10 + (i));
////			printf("0x%02x ",curr_pkt_data);
////    	}
////    	printf("\n");
//
//    	//*((uint16_t *) read_pointer_addr) = (uint16_t) (read_pointer + PACKET_SIZE) % ADDR_LIMIT;
//    }
//
//    fflush(stdout);

    if(munmap(map_base, MAP_SIZE) == -1) {
       printf("Failed to unmap memory");
       return -1;
    }
    close(fd);
    return 0;
}
