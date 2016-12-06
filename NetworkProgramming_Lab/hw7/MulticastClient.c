//setup
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 255
#define MULTICAST_GROUP "226.1.2.3" 

void ErrorReport(char *mess)
{
	perror(mess);
}

//declare
int main(int argc, char *argv[])
{
	int sock; //socket descriptor
	struct sockaddr_in echoserver;
	struct sockaddr_in echoclient;
	struct ip_mreq group;
	char buffer[BUFFSIZE];
	unsigned int serverlen, clientlen;
	int received = 0;
	
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <port_remote> \n", argv[0]);
		return 1;
	}
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		ErrorReport("Failed to create socket.");
	}
	
	//setup to allow multiple instances of this application to receive multicast datum
	int reuse =1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse));

	//construct a server socketaddr_in structure
	memset(&echoserver, 0, sizeof(echoserver));		    //clear strcture
	echoserver.sin_family = AF_INET;
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY);     //IP address
	echoserver.sin_port = htons(atoi(argv[1]));         //remote port
		
	//bind the socket
	serverlen = sizeof(echoserver);
	if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0)
	{
		ErrorReport("Failed to bind server socket.");
	}

	/*join the multicast group "226.0.0.1" on the local 172.16.5.31 interface.*/
	/*Note that this IP_ADD_MEMBERSHIP option must be called for each local interface over which the multicast*/
	/*datagrams are to be received.*/
	group.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
	/*group.imr_interface.s_addr = htonl(INADDR_ANY);*/
	group.imr_interface.s_addr = inet_addr("172.16.5.31");
	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &group, sizeof(group));

	memset(&buffer, 0, sizeof(buffer));
	while (1)
	{
		//received the brocast message from server
		clientlen = sizeof(echoclient);
		fprintf(stdout, "received from server: ");
		if ((received = recvfrom(sock, buffer, BUFFSIZE, 0, (struct sockaddr *) &echoclient, &clientlen)) < 0)
		{
			ErrorReport("Failed to receive message.");
		}
		/*
		if (read(sock, buffer, BUFFSIZE) < 0)
		{
			ErrorReport("Failed to receive message.");
		}
		*/
		buffer[received] = '\0'; //assure null-terminated string
		fprintf(stdout, buffer);
		fprintf(stdout,"\nserver IP: %s\n", inet_ntoa(echoclient.sin_addr));
	}
	close(sock);
	return 0;
}
