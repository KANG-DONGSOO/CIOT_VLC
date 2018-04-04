#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
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

#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)

unsigned int readFileValue(char filename[]){
   FILE* fp;
   unsigned int value = 0;
   fp = fopen(filename, "rt");
   fscanf(fp, "%x", &value);
   fclose(fp);
   return value;
}

int main(int argc, char *argv[]){

	int fd,i;
	unsigned int mmap_addr = readFileValue(MMAP0_LOC "addr");
	unsigned long read_pointer, write_pointer;
	void *map_base, *write_pointer_addr, *read_pointer_addr, *curr_pointer;
	off_t target = mmap_addr;

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

	*((uint16_t *) write_pointer_addr) = (uint16_t) 0x0;
	*((uint16_t *) read_pointer_addr) = (uint16_t) 0x0;

	for(;;){

		do{
			read_pointer = *((uint16_t *) read_pointer_addr);
			write_pointer = *((uint16_t *) write_pointer_addr);
			//printf("buffer is full\n");
		} while((write_pointer+10) % 10000 == read_pointer);

		curr_pointer = map_base + (target+6+write_pointer & MAP_MASK);
		*((uint16_t *) curr_pointer) = 0x0004;
		*((uint16_t *) curr_pointer+2) = 0x0009;
		*((uint16_t *) curr_pointer+4) = 0x0012;
		*((int *) curr_pointer+6) = 123;
//		for(i==0; i<4;i++){
//			*((uint16_t *) curr_pointer+10+(i*2)) = i;
//		}

		printf("writing data, read_pointer is %d and write_pointer is %d\n", read_pointer, write_pointer);
		*((uint16_t *) write_pointer_addr) = (uint16_t) (write_pointer+ 10) % 10000;
	}


	return 0;
}
