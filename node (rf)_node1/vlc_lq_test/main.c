#include <stdio.h>


int main(){
	int i;
	double lq = 0.14f;

	int in_255_range = (int)(lq * (double) 255);
	printf("lq in int : %d\n",in_255_range);
	for(i=0;i<8;i++){
		if(in_255_range>>i & 1)
			printf("1 ");
		else
			printf("0 ");
		printf("\n");
	}

	printf("direct casting: %u\n",(unsigned char)in_255_range);
	return 0;
}
