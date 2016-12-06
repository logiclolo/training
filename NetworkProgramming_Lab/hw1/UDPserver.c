//setup
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 255


//declare
int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in echoserver;
	struct sockaddr_in echoclient;
	char buffer[BUFFSIZE];
	unsigned int echolen, clientlen, serverlen;
	int received = 0;
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
		return 1;
	}
	//create the UDP socket
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("Failed to create socket.");
	}
	//construct a server socketaddr_in structure
	echoserver.sin_family = AF_INET;
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY); //any IP address
	echoserver.sin_port = htons(atoi(argv[1]));
	//bind the socket
	serverlen = sizeof(echoserver);
	if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0)
	{
		perror("Failed to bind server socket.");
	}
	memset(&buffer, 0, sizeof(buffer));
	//the received/send loop
	while(1)
	{
		//received a message from the client
		clientlen = sizeof(echoclient);
		if ((received = recvfrom(sock, buffer, BUFFSIZE, 0, (struct sockaddr *) &echoclient, &clientlen)) < 0)
		{
			perror("Failed to receive message.");
		}
		fprintf(stderr, "Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
		fprintf(stderr,"%s\n",buffer);
		//send the message back to client
		if (sendto(sock, buffer, received, 0, (struct sockaddr *) &echoclient, sizeof(echoclient)) != received)
		{
			perror("Mismatch in number of echo'd bytes.");
		}
	}
	return 0;
}
