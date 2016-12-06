#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <fcntl.h>

#define MAXPENDING 5
#define BUFFSIZE 1024
#define CONNECTNUM 64
#define SOCKETOFF 1 
#define SOCKETON 0

int HandleMessage(int sock)
{

	char buffer[BUFFSIZE];
	int received = -1;
	memset(&buffer, 0 , sizeof(buffer));

	printf("enter recv\n");
	received =recv(sock, buffer, BUFFSIZE, 0);
	if (received < 0) /* received = -1, bad file descriptor*/
	{
		printf("Failed to receive initial bytes from client: %d\n", received);
	}
	else if (received == 0) /* received = 0, client left without notification*/
	{
		close(sock);
		return SOCKETOFF;
	}



	//send back received data
	if (send(sock, buffer, received, 0) != received)
	{
		perror("Failed to send bytes to client.");
	}
	else
	{
		//printf("%s\n",buffer);
		if(!strcmp(buffer,"exit"))
		{
			close(sock);
			return SOCKETOFF;
		}
		

	}	
		return SOCKETON;
}

int main(int argc, char *argv[])
{
	int socknum = 0;
	int serversock, clientsock[CONNECTNUM];
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
	


	
	int flag;
	int cliSock;
	struct timeval waitd;           //the MAX wait time for an event
	waitd.tv_sec = 5;               //make select wait up to 5 second for data
	waitd.tv_usec = 0;              //and 0 milliseconds

	int numsocket = 0;
	int fd = 0, fd_hwm = 0;
	fd_set set_flags, read_set_flags;
	//int nfds = getdtablesize();
	int nfds = 0;

	nfds = serversock;	
	FD_ZERO(&set_flags);
	FD_SET(serversock, &set_flags);
	
	
	while(1)
	{
		int rc;	
		read_set_flags = set_flags;
		unsigned int clientlen = sizeof(echoclient);
		
		printf("enter select\n");
		rc = select(nfds+1, &read_set_flags, NULL, NULL, NULL);
		if (rc > 0)
		{
			for (fd = 0; fd <= nfds; fd++ )
			{
				//printf("fd is %d,nfds is %d\n",fd,nfds);
				//fflush(stdout);
				if (FD_ISSET(fd, &read_set_flags))
				{
       				//FD_CLR(fd, &read_set_flags);//it means the socket is ready to use read()
					printf("fd is %d,nfds is %d\n",fd,nfds);
					fflush(stdout);
					if (fd == serversock)
					{
							if ((cliSock = accept(serversock, (struct sockaddr *) &echoclient, &clientlen)) < 0)
							//if ((clientsock[numsocket] = accept(serversock, (struct sockaddr *) &echoclient, &clientlen)) < 0)
							{
								perror("Failed to accept client connection.");
							}
							else
							{
								if (cliSock > nfds)
								{
									nfds = cliSock;
								}
								
								printf("cliSock is %d\n",cliSock);
								FD_SET(cliSock, &set_flags);
//								flag = fcntl(cliSock, F_GETFL, 0);
//								fcntl(cliSock, F_SETFL, flag | O_NONBLOCK);
								fprintf(stdout, "Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
								//HandleMessage(cliSock);
							}
					}
					else
					{			
							if (HandleMessage(fd) == SOCKETOFF)
							{
								FD_CLR(fd, &set_flags);
							}
					}
				}
			}
		}
		else if(rc < 0)
				perror("select");
	}
}
