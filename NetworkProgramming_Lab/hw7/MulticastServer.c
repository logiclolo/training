#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MULTICAST_GROUP "226.1.2.3"
#define BUFFSIZE 700

int main(int argc, char *argv[])
{
	int sock;
	struct in_addr local;
	struct sockaddr_in echoserver;
	unsigned int BRdatalen;
	char buffer[BUFFSIZE];
	
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
		return 1;
	}
	//create the UDP socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("Failed to create socket.");
	}
	
	//construct a server socketaddr_in structure
	memset(&echoserver, 0, sizeof(echoserver));
	echoserver.sin_family = AF_INET;
	echoserver.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
	echoserver.sin_port = htons(atoi(argv[1]));
	
	//ask for multicast message
	printf("input multicast message:");
	memset(&buffer,0,sizeof(buffer));
	scanf("%s", buffer);

	//disable the data lookback so you do not receive your own datagrams.
	//char lookback = 0;
	//setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &lookback, sizeof(lookback));

	//set local interface for outbound multicast datagrams.
	//local.s_addr = htonl(INADDR_ANY);
	/*local.s_addr = inet_addr("172.16.5.31");*/
	local.s_addr = inet_addr("172.16.5.31");
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *) &local, sizeof(local));

	BRdatalen = strlen(buffer);
	//broadcast send the data to clients
	while (1){
		if (sendto(sock, buffer, BRdatalen, 0, (struct sockaddr *) &echoserver, sizeof(echoserver)) != BRdatalen)
		{
			perror("Sent a different number of bytes than expected.");
		}
		sleep(5);
	}
	
	return 0;
}
