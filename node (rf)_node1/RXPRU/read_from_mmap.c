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
#define ECC_LEN 2

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
    unsigned long read_pointer, flag;
	void *map_base, *write_pointer_addr, *flag_pointer_addr, *data_pointer_addr, *curr_pointer;
    uint8_t self_id1 = 0,self_id2 = 0, dest_id = 0;
    uint16_t self_id = 0,length = 0,seq_number1 = 0, seq_number2 = 0,read_idx = 0,write_idx=0;
    uint32_t seq_number = 0;
    uint8_t data = 0;
    uint32_t offset = 0;

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


    data_pointer_addr = map_base + (target & MAP_MASK);

//		HOWTO WRITE TO A MEMORY ADDRESS:
//    	*((uint16_t *) read_pointer_addr) = (uint16_t) 40;


   // for(;;){
    offset = 1;
    for(;;){
    	read_idx = *((uint16_t *) data_pointer_addr + offset);
		write_idx = *((uint16_t *) data_pointer_addr);
		printf("Read idx: %d\nWRite Idx: %d\n\n",read_idx,write_idx);
    }

//    read_idx = 4;
//    printf("Write Index: %d\n",read_idx);
//
//    offset = read_idx / sizeof(uint16_t);
//	length = *((uint16_t *) data_pointer_addr + offset);
//	printf("Length: %d\n",length);
//	offset++;
//	self_id = *((uint16_t *) data_pointer_addr + offset);
//	printf("Self ID: %d\n",self_id);
//	offset++;
//	dest_id = *((uint16_t *) data_pointer_addr + offset);
//	printf("Dest ID: %d\n",dest_id);
//	offset++;
//	seq_number1 = *((uint16_t *) data_pointer_addr + offset);
//	offset++;
//	seq_number2 = *((uint16_t *) data_pointer_addr + offset);
//	seq_number = (seq_number2<<16) + seq_number1;
//	printf("Seq NUmber: 0x%08x\n",seq_number);
//	offset++;
//
//	offset*=2;
//	printf("Data: ");
//	for(i=0;i<length;i++){
//		data = *((uint8_t *) data_pointer_addr+offset);
//		printf("0x%02x ",data);
//		offset++;
//	}
//	printf("\nECC: ");
//	for(i=0;i<ECC_LEN;i++){
//		data = *((uint8_t *) data_pointer_addr+offset);
//		printf("0x%02x ",data);
//		offset++;
//	}

	printf("\n");

	//printf("\nCurr offset: %d\n",offset);

    if(munmap(map_base, MAP_SIZE) == -1) {
       printf("Failed to unmap memory");
       return -1;
    }
    close(fd);
    return 0;
}
