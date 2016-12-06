//client setup
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <error.h>

#define BUFFSIZE 1024

int main(int argc, char *argv[])
{
	int sock;
	int Blen;
	struct sockaddr_in echoserver;
	char buffer[BUFFSIZE];
	unsigned int echolen;
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: TCPbroadcast <port>\n");
		return 1;
	}
	//create the TCP socket
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("Failed to create socket.");
	}
	//establish the connection
	//construct the server sockaddr_in structure
	memset(&echoserver, 0, sizeof(echoserver));
	echoserver.sin_family = AF_INET;                 //Internet/IP
	//netmask 255.255.0.0 <---> broadcast addr xx.xx.255.255
	//netmask 255.255.255.0 <---> broadcast addr xx.xx.xx.255
	echoserver.sin_addr.s_addr = inet_addr("192.168.255.255"); //IP address
	echoserver.sin_port = htons(atoi(argv[1]));      //server port
	
	//message for broadcast
	printf("input message for broadcast:");
	memset(&buffer,0,BUFFSIZE);
	scanf("%s",buffer);

	//allow broadcast
	int broadcast = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	
	Blen = strlen(buffer);	
	//send the data to server
	if (sendto(sock, buffer, Blen, 0, (struct sockaddr *) &echoserver, sizeof(echoserver)) != Blen)
	{
		perror("Mismatch in number of sent bytes.");
	}
	
	
	return 0;
}
