/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 1000

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes,ack_bytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    while(1){
    	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		printf("listener: got packet from %s\n",
			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s));
		printf("listener: packet is %d bytes long\n", numbytes);

		int idx = 0, printout;
		printout = (int) buf[idx] * 256 + (int)buf[idx+1];
		printf("Packet byte checking:\nnode_id: %d\n",printout); idx+=2;
		/*temp.ACK_number = (int) report_bytes[idx] * 256 + (int) report_bytes[idx+1];
		printf("ACK Number: %d\n",temp.ACK_number); idx+=2;

		int pattern_length = (int) report_bytes[idx] * 256 + (int) report_bytes[idx+1];
		idx+=2;
		printf("Pattern Length:  %d\nPattern:\n",pattern_length);

		temp.ACK_bit_pattern = (char *) malloc(pattern_length * sizeof(char));
		for(i=idx;i<pattern_length;i++){
			temp.ACK_bit_pattern[i] = (char)report_bytes[idx];
			printf("%c ",temp.ACK_bit_pattern[i]);
			idx++;
		}
		printf("\n");

		int vlc_tx_number = (int) report_bytes[idx] * 256 + (int) report_bytes[idx+1];
		printf("VLC TX Number: %d\n",vlc_tx_number); idx+=2;*/

		//printf("listener: packet contains \"%s\"\n", buf);
		/*if((ack_bytes = sendto(sockfd,"ack",strlen("ack"),0,(struct sockaddr *)&their_addr,addr_len)) == -1){
			perror("failed ack");
			exit(1);
		}*/
    }

    close(sockfd);

    return 0;
}
