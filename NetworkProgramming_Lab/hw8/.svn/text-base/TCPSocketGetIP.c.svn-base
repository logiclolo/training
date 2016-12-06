#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFSIZE 4096

int SearchIndex(char targetArray[], char searchArray[], int index)
{
	int i;
	int len = strlen(targetArray);
	char tempArray[len];

	memset(&tempArray, 0, len);
	//printf("targetArray::%s\n",targetArray);
	
	while (index <= strlen(searchArray))
	{
		for(i = 0; i < len; i++)
		{
			tempArray[i] = searchArray[index+i];
		}
		if(strcmp(tempArray,targetArray) == 0) /*match*/
		{
			return index;
		}
		else /*mismatch*/
		{
			index++;
		}
	}

	return -1;
}
	



void ErrorReport(char *mess)
{
	perror(mess);
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in gethttpsock;
	//int status;
	
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: gethttpsock <port=80>\n");
		return 1;
	}

	//create the TCP socket
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		ErrorReport("Failed to create socket.");
	}
	struct hostent *phe;

	phe = gethostbyname("checkip.dyndns.org");//incoming address, call by get host name function
	//construct the server sockaddr in structure
	memset(&gethttpsock, 0, sizeof(gethttpsock));         
	gethttpsock.sin_family = AF_INET;
	gethttpsock.sin_addr = *((struct in_addr *)phe->h_addr);   //IP
	gethttpsock.sin_port = htons(atoi(argv[1]));               //port


	if (connect(sock, (struct sockaddr *) &gethttpsock, sizeof(gethttpsock)) < 0)
	{
		ErrorReport("Failed to connect with server.");
	}

    //send the request

	static const char protocolHead[] =
	"\
	GET / HTTP/1.1\r\n\
	Accept-Language: zh-TW\r\n\
	Host: checkip.dyndns.org\r\n\
	Connection: Keep-Alive\r\n\r\n\
	";
	
	
	if (send(sock, protocolHead, strlen(protocolHead), 0) != strlen(protocolHead))
	{
		ErrorReport("Mismatch in number of sent bytes.");
	}
	
    char buffer[BUFFSIZE];
	memset(&buffer, 0 , sizeof(buffer));
	
	int bytes = 0;
	if ((bytes = recv(sock, buffer, BUFFSIZE-1, 0)) < 1)
	{
		ErrorReport("Failed to receive bytes from server.");
	}
	buffer[bytes] = '\0'; //assure null terminated string
	
	//fprintf(stdout, "buffer = %s", buffer);

	char htmlbodyHead[6] = "<body>";
	char htmlbodyTail[7] = "</body>";
	char checktemp[6], checktemp2[7];
	int checknum = 0, checknum2 = 0;
	int headIndex = 0, tailIndex = 0;
	int i;
	
	headIndex = SearchIndex(htmlbodyHead,buffer,headIndex);
	headIndex = headIndex + 6; /*skip <body>*/
	tailIndex = SearchIndex(htmlbodyTail,buffer,headIndex);	

	for (i = headIndex; i < tailIndex; i++)
	{
		printf("%c",buffer[i]);
	}
	printf("\n");
	fflush(stdout);
	
	return 0;
}


