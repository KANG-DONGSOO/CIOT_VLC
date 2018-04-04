#include <stdio.h>
#include <stdlib.h>

int generate_m(int *vlc_ids, int n_vlc){
	int i;
	int retval = 0;

	for(i=0;i<n_vlc;i++){
		int temp = 1;
		temp = temp<<(vlc_ids[i]-1);
		printf("temp: %d\n",temp);
		retval = retval | temp;
		printf("retval: %d\n\n",retval);
	}

	return retval;
}

int is_k_in_sn(int vlc_id,int sn){
	if(sn<=0)
		return 0;

	return ((1<<(vlc_id-1))&sn) > 0;
}

int main(){
	int n_vlc = 3,i;
	int * vlc_ids = (int *) malloc(n_vlc * sizeof(int));
	vlc_ids[0] = 1;
	vlc_ids[1] = 2;
	vlc_ids[2] = 3;
	//vlc_ids[3] = 4;

	printf("Generated M: %d\n", generate_m(vlc_ids,n_vlc));

	if(is_k_in_sn(2,-1))
		printf("in\n");
	else
		printf("not\n");

	int sn = 2;
	int n_k = 0;
	int max_vlc = 4;
	int *k = (int *) malloc(max_vlc * sizeof(int));
	int temp_id = 1;
	while(sn>0){
		int temp = sn & 1;
		if(temp > 0){
			k[n_k] = temp_id;
			n_k+=1;
		}
		sn = sn>>1;
		temp_id+=1;
	}
	printf("Result aray k from sn: ");
	for(i=0;i<n_k;i++){
		printf("%d ",k[i]);
	}
	printf("\n");
	return 0;
}
