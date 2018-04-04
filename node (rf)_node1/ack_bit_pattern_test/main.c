#include <stdio.h>
#include <stdlib.h>

int main(){
	int pattern_bytes,pattern_length,len,unsigned_char_size,i,j;
	int remaining_pattern_length = pattern_length;
	int cur_last_idx = unsigned_char_size,cur_idx;
	unsigned char patterns[1000];
	unsigned char *result;
	unsigned char convert[1000];

	pattern_length=0;
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';

	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '1';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';

	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '0';
	patterns[pattern_length++] = '1';

	unsigned_char_size = sizeof(unsigned char) * 8;

	printf("size of unsigned char: %d bits\n",unsigned_char_size);

	pattern_bytes = pattern_length / unsigned_char_size + (pattern_length % unsigned_char_size == 0 ? 0 : 1);

	result = (unsigned char*) malloc(pattern_bytes * sizeof(unsigned char));

	printf("number of unsigned char needed: %d\n",pattern_bytes);

	remaining_pattern_length = pattern_length;

	for(i=0;i<pattern_bytes;i++){
		unsigned char temp_byte = (unsigned char) (0 & 0x0);

		cur_idx = i * unsigned_char_size;

		if(remaining_pattern_length > unsigned_char_size){
			cur_last_idx = cur_idx + unsigned_char_size;
		}
		else{
			cur_last_idx =  cur_idx + remaining_pattern_length;
		}

		//printf("cur idx: %d\ncur last idx: %d\n",cur_idx,cur_last_idx);

		int n_shift = unsigned_char_size - 1;

		for(j=cur_idx;j<cur_last_idx;j++){
			//printf("temp byte before: %08x\n",temp_byte);
			//printf("from pattern after shift: %08x\n",(patterns[j]& 0x1)<<n_shift);
			temp_byte = temp_byte | (patterns[j]& 0x1)<<n_shift; //Convert each character (0 or 1) in the packet bit pattern to a bit
			n_shift--;
			//printf("temp byte after: %08x\n\n",temp_byte);
		}

		printf("pattern byte %d: %08x\n---------\n",i,temp_byte);
		//buff[idx] = temp_byte; idx++;
		result[i] = temp_byte;
		remaining_pattern_length -= unsigned_char_size;
	}

	//CONVERTING BACK TO BIT ACK PATTERN
	int convert_idx = 0;
	int conv_remaining_pattern = pattern_length;
	int conv_shift_until;

	for(i=0;i<pattern_bytes;i++){
		unsigned char temp = result[i];
		if(conv_remaining_pattern >= unsigned_char_size){
			conv_shift_until = 0;
		}
		else{
			conv_shift_until  = unsigned_char_size - conv_remaining_pattern;
		}
		for(j=(unsigned_char_size-1);j>=conv_shift_until;j--){
			convert[convert_idx++] = (unsigned char) temp>>j & 0x1;
		}
		conv_remaining_pattern-=unsigned_char_size;
	}

	for(i=0;i<pattern_length;i++){
		printf("%u ",convert[i]);
	}
	printf("\n");

	return 0;
}
