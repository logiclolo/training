#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define MAXPENDING 0
#define BUFFSIZE 1024

//the connection handler
void HandleClient(int sock)
{
	char buffer[BUFFSIZE];
	int received = -1;
	memset(&buffer, 0 , sizeof(buffer));

	while(1)
	{
			//received data
			if ((received = recv(sock, buffer, BUFFSIZE, 0)) < 0)
			{
				perror("Failed to receive initial bytes from client.");
			}
			//send bytes and check the incoming data		
			//send back received data
			if (send(sock, buffer, received, 0) != received)
			{
				perror("Failed to send bytes to client.");
			}
			//received = 0;

			if(!strcmp(buffer,"exit"))
			{
				close(sock);
				break;
			}
			fprintf(stderr,"%s\n",buffer);
			memset(&buffer, 0 , sizeof(buffer));
	}
}

int main(int argc, char *argv[])
{
	int socknum = 0;
	int serversock, clientsock;
	struct sockaddr_in echoserver, echoclient;
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: echoserver <port>\n");
		return 1;
	}
	//create the TCP socket
	if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("Failed to create socket.");
	}

	//construct the server sockaddr in structure
	memset(&echoserver, 0, sizeof(echoserver));         
	echoserver.sin_family = AF_INET;                   //Interner/IP
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY);    //incoming address
	echoserver.sin_port = htons(atoi(argv[1]));        //server port
	
	//binding and listening
	//bind the server socket
	if (bind(serversock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0)
	{
		perror("Failed to bind the server socket.");	
	}
	//listen on the server socket
	if (listen(serversock, MAXPENDING) < 0)
	{
		perror("Failed to listen on server socket.");	
	}
	
	//socket factory
	//run until cancelled
	while(1)
	{
		unsigned int clientlen = sizeof(echoclient);
		//wait for client connection
		if ((clientsock = accept(serversock, (struct sockaddr *) &echoclient, &clientlen)) < 0)
		{
			perror("Failed to accept client connection.");
		}
		fprintf(stdout, "Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
		HandleClient(clientsock);
	}
}
