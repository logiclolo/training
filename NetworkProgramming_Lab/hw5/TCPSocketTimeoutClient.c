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
#define INPUTSTRING "input message (type \"exit\" to left):"

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in echoserver;
	char buffer[BUFFSIZE];
	char tmp[BUFFSIZE];
	unsigned int echolen;
	int received = 0;
	if (argc != 3)
	{
		fprintf(stderr, "USAGE: TCPecho <server_ip> <port>\n");
		return 1;
	}
	//create the TCP socket
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("Failed to create socket.");
	}
	//establish the connection
	//construct the server sockaddr_in structure
	memset(&echoserver, 0, sizeof(echoserver));
	echoserver.sin_family = AF_INET;                 //Internet/IP
	echoserver.sin_addr.s_addr = inet_addr(argv[1]); //IP address
	echoserver.sin_port = htons(atoi(argv[2]));      //server port
	
	//the three line of codes below must put before connect()
	printf(INPUTSTRING);
	scanf("%s", buffer);
	echolen = strlen(buffer);


	//timeout setting
	struct timeval waitd;
	waitd.tv_sec = 2;
	waitd.tv_usec = 0;
	int timelen = sizeof(waitd);

	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &waitd, timelen);
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &waitd, timelen);
	
	if (connect(sock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0)
	{
		perror("Failed to connect with server.");
	}

	//printf(INPUTSTRING);
	//scanf("%s", &buffer);
	//echolen = strlen(buffer);
	
	//send the data to server
	if (send(sock, buffer, echolen, 0) != echolen)
	{
		perror("Mismatch in number of sent bytes.");
	}
	
	//receive the data back from the server
	while (1){//
		while (received < echolen)
		{
			int bytes = 0;
			
					
			if ((bytes = recv(sock, buffer, BUFFSIZE-1, 0)) < 1)
			{
				if (bytes == 0)
				{
					printf("connection is over\n");
					exit(0);
				}
				perror("Failed to receive bytes from server.");
			}

			received += bytes;
			buffer[bytes] = '\0';
			//print out message on screen
			sprintf(tmp, "message from server: %s\n", buffer);
			printf("%s", tmp);
		}
	
		if (!strcmp(buffer,"exit"))
		{
			close(sock);
			break;
		}
		memset(&buffer, 0, sizeof(buffer));
		memset(&tmp, 0, sizeof(tmp));
		received = 0;

		printf(INPUTSTRING);
		scanf("%s", buffer);
		echolen = strlen(buffer);
		if (send(sock, buffer, echolen, 0) != echolen)
		{
			perror("Mismatch in number of sent bytes.2");
		}
	}
	
	return 0;
}
