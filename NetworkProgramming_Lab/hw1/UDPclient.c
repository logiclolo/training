#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>

#define BUFFSIZE 700 

int main(int argc,char *argv[])
{
	int    sock;
	struct sockaddr_in echoserver;
	struct sockaddr_in echoclient;
	char msg_buf[BUFFSIZE];
	struct hostent *phe;  /*pointer to host information host*/ 
 	char send_text[] = "Hello,logic!";
	int echolen;
	int received;
	int clientlen;
	
	if(argc != 4)
 	{
 		fprintf(stderr,"Usage : client <server ip> <port> <specific port>\n");
        exit(1);
 	}    

                              
 	if ((sock= socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0)
	{
		perror("Can't create socket!\n");
	}
	
	//construct a server socketaddr_in struct 
 	bzero(&echoserver,sizeof(echoserver));
 	echoserver.sin_family = AF_INET;
 	echoserver.sin_addr.s_addr = inet_addr(argv[1]); 
 	echoserver.sin_port = htons(atoi(argv[2]));
	
	echolen = strlen(send_text);
	if (sendto(sock, send_text, echolen, 0, (struct sockaddr *) &echoserver, sizeof(echoserver)) != echolen)
	{
		perror("Can't send message to server!\n");
	}
	

	//reveive message from server
	fprintf(stdout, "received from server:");
	clientlen = sizeof(echoclient);	
	memset(&msg_buf,0,sizeof(msg_buf));
	if ((received = recvfrom(sock, msg_buf, BUFFSIZE, 0, (struct sockaddr *) &echoclient, &clientlen)) != echolen)
	{
		perror("mismatch in number of bytes!\n");
	}
	
	//check that client and server use the same socket
	if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr)
	{
		//perror("Received a packet from an unexpected server!\n");
		printf("%d,%d\n",echoserver.sin_addr.s_addr,echoclient.sin_addr.s_addr);
	}

	msg_buf[received] = '\0';
	fprintf(stdout,msg_buf);
	fprintf(stdout,"\n");
	close(sock);
	
	
	return 0;
	
}      
  
