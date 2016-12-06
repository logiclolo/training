#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>


#define PORT 		0x1234
#define STR_MAX         1024
#define SOCKET_ERROR    -1
#define TRUE		1
#define FALSE		0
#define UNFIND		0
#define	FIND		1
#define MAX_ARGV_NUM	32
#define	EXIT		2
#define MAX_PIPE_NUM	1024
#define MAX_BUF 2048


#define CONNECT 1
#define BIND 2



#define F_CONNECTING 0
#define S_READING 1
#define S_WRITING 2
#define S_DONE 3
#define C_READING 4
#define C_WRITING 5
#define C_DONE 6


struct cmd_struct {
	char **argv ;
	int in ;
	int out ;
	int err ;
	int pipe_cross_msg ;
	int pipe_cross_num ;
	char filename[64] ;
} ;

struct Sock4 {
	unsigned char vn ;
	unsigned char cd ;
	unsigned int dst_port ;
	//unsigned int dst_ip ;
	unsigned char dst_ip[32] ;
	unsigned int sr_port ;
	unsigned char sr_ip[32] ;
	char user_id[32] ;
	char domain_name[32] ;

};

struct Sock4 mySock;

typedef struct ClientStruct {
	char ip[32] ;
	int port ;
}clientStruct;

typedef struct cmd_struct CMD_STRUCT ;

void sig_chld(int);     /* prototype for our SIGCHLD handler */
int send_msg_to_cli(int cliSock, char *msg) ;
int decode_msg(int cliSock, char *msg) ;
int check_cmd(int cliSock, char *str) ;
int check_exit(int cliSock, char *str) ;
int create_one_level_pipe(int cliSock, CMD_STRUCT *cmd_list, int cmd_list_num) ;
char *env;
int *pipe_table[MAX_PIPE_NUM] ;
int cur_pos = -1, cmd_list_num=1;
unsigned char buf[MAX_BUF];
unsigned char buftmp[MAX_BUF];
int server_fd,server_fd1,socks_fd,n,error;
int sucks_status;
int bindflag;
//struct hostent *he;
//struct sockaddr_in client_sin;

CMD_STRUCT *cmd_list ;


char welcome[] = 
"\
****************************************\n\
** Welcome to the information server. **\n\
****************************************\n\
" ;

int recv_msg(int from,int to,int server_or_client)
{
    
	int size = 9000;
	char buf[size],*tmp,*a;
	int len,n,i;
	
	
	//memset(buf,'\0',6000);
	if((len=read(from,buf,sizeof(buf)-1)) <0) 
		return -1;
	
	buf[len] = 0;
	
	
	if(len > 0){
		if(server_or_client)
			printf("\npid=%u,read from server-----------------------status is %d,len is %d,str(buf) is %d,buf[0] = %d,read is:\n%s\n",getpid(),sucks_status,len,strlen(buf),buf[0],buf);
		else
			printf("\npid=%u,read from client-----------------------status is %d,len is %d,str(buf) is %d,buf[0] = %d,read is:\n%s\n",getpid(),sucks_status,len,strlen(buf),buf[0],buf);
		
		fflush(stdout);
	}
	

	if(len>0)
	{
		
		
		n = write(to,buf,len);
		printf("write bytes is %d\n",n);
		fflush(stdout);
		if( n == -1)
			printf("errno is %d\n",errno);
	}
	
	
	
	fflush(stdout); 
	return len;
}

int decode_socks_request(){

			//1.decode request-----------------------------
			

			
			int mode;
			
			int package_size = 8;
			unsigned char package[package_size];
			unsigned char package_to_ftp[30];
			unsigned char ip_address[30];

			unsigned char VN = buf[0];
			unsigned char CD = buf[1];
			unsigned int DST_PORT = (buf[2] << 8) | buf[3];
			unsigned int DST_IP = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
			char * USER_ID = buf + 8;
			
			//connect_mode or bind_mode -------------
			if(CD == 1)
				mode = CONNECT;
			if(CD == 2)
				mode = BIND;

			printf("mode(connect=1/bind=2) is %d\n",mode);
			fflush(stdout);

			
			printf("VN: %u\n",mySock.vn = VN);
			printf("CD: %u\n",mySock.cd = 90);
			printf("D_PORT:%u\n",mySock.dst_port = DST_PORT);
			sprintf(mySock.dst_ip,"%u.%u.%u.%u",buf[4],buf[5],buf[6],buf[7]);
			printf("D_IP:%s\n",mySock.dst_ip);
			printf("S_PORT:%u\n",mySock.sr_port);
			printf("S_IP:%s\n",mySock.sr_ip);
			fflush(stdout);

			return mode;

}

int check_firewall(){

	if(!strncmp(mySock.sr_ip,"140.114",7) || !strncmp(mySock.sr_ip,"140.113",7))
		return 1;
	else
		return 0;


}


int main(int argc, char *argv[])
{
	int srvSock ;
	int cliSock ;
	struct sockaddr_in  srvSockaddr;
	struct sockaddr_in  localSockaddr ;
	struct sockaddr_in  peerSockaddr ;
	struct sockaddr_in  cliSockaddr ;
	socklen_t nsockaddrsize ;
	char strCliHost[STR_MAX] ;
	char strLocalHost[STR_MAX] ;
	int nCliPort ;
	int nLocalPort ;
	int nSrvPort ;
	int nRetValue ;
	int nByteRead, nByteWrite ;
	char strEcho[STR_MAX] ; /* used for incomming data, and
                                        outgoing data */
	clientStruct new_cli;

	fd_set readfds ;
    //char buf[1024] ;
	char symbol[] = "% ", exit_msg[] = "exit \n" ;
	char *env ;
	int i=0 ; 
	char *temp ;
	char *ip;
	char resolved_path[64] ;
	pid_t pid ;
        int status, fp ;

	struct sigaction act;
	/* Assign sig_chld as our SIGCHLD handler */
	act.sa_handler = sig_chld;

	/* We don't want to block any other signals in this example */
	sigemptyset(&act.sa_mask);

	/*
	 *      * We're only interested in children that have terminated, not ones
	 *           * which have been stopped (eg user pressing control-Z at terminal)
	 *                */
	act.sa_flags = SA_NOCLDSTOP;

	/*
	 *      * Make these values effective. If we were writing a real 
	 *           * application, we would probably save the old value instead of 
	 *                * passing NULL.
	 *                     */
	//if (sigaction(SIGCHLD, &act, NULL) < 0) 
	//{
	//	fprintf(stderr, "sigaction failed\n");
	//	return 1;
	//}

	if(argc < 2)
    	{
        	printf("\nUsage: tcp_echosrv host-port\n") ;
        	return 0;
    	}

	nSrvPort = atoi(argv[1]) ;

	/* get an internet domain socket */
	printf("\n[001] Creating service socket...") ;
	srvSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
	if(srvSock == SOCKET_ERROR)
	{
		printf("\n[ERR] Can't create service socket\n") ;
		return 0;
	}
	printf("\n[002] Complete service socket creating") ;

	/* complete the socket structure */
	printf("\n[003] Set service socket info") ;
	srvSockaddr.sin_addr.s_addr = INADDR_ANY ;
	srvSockaddr.sin_port = htons(nSrvPort) ;
	srvSockaddr.sin_family = AF_INET ;

	/* bind the socket to the port number */
	printf("\n[004] bind the service socket to the PORT %d", nSrvPort) ;
	nRetValue = bind(srvSock, (struct sockaddr*)&srvSockaddr, sizeof(srvSockaddr)) ;
	if (nRetValue == SOCKET_ERROR)
	{
		printf("\n[ERR] Can't bind the sock to port\n") ;
		close(srvSock) ;
		return 0;
	}
	printf("\n[005] Finish binding service socket to the PORT %d", nSrvPort) ;

	/* show that we are willing to listen */
	printf("\n[006] Service socket is listening") ;
	nRetValue = listen(srvSock, 1) ;
	if (nRetValue == SOCKET_ERROR)
	{
		printf("\n[ERR] Fail to service socket listen\n") ;
		close(srvSock) ;
		return 0;
	}

	printf("\n[007] Service socket listening Completed") ;
	printf("\n[008] Wainting for client...") ;
	fflush(stdout);
	while(1) {
		/* Waiting for a incoming connection */
		cliSock=accept(srvSock, (struct sockaddr*)&cliSockaddr, &nsockaddrsize) ;
		//printf("cliSock is %d\n",cliSock);
		if (cliSock == SOCKET_ERROR )
		{
			printf("\n[ERR] Can't accept client connection\n") ;
			close(srvSock) ;
			return 0;
		}
		else
			printf("\n[009] Accept one TCP ECHO CLIENT,clisock is %d\n",cliSock) ;

		pid = fork();
		if(pid < 0){
			printf("fork() error\n");
			exit(1);
		}
		else if(pid == 0){ // child process
			
			// set path
			/*
			setenv("PATH", "bin:.", 1) ;
			realpath("./ras", resolved_path) ;
			chdir(resolved_path) ;
			*/
			printf("[011] Waiting for client message") ;
			printf("*******\n");
			
			int socks_mode;
			int access;
			//clientStruct new_cli;
			ip = inet_ntoa(cliSockaddr.sin_addr);

			sprintf(new_cli.ip,"%s",ip) ;
			new_cli.port = ntohs(cliSockaddr.sin_port) ;
			mySock.sr_port = new_cli.port;
			strcpy(mySock.sr_ip,new_cli.ip);

			memset(buf,'\0',strlen(buf));
			
			nByteRead = recv(cliSock,buf,MAX_BUF,0);
			buf[nByteRead] = '\0';
			for(i=0;i<(nByteRead-1);i++){
				printf("read bytes %d,read message[%d]:%u\n",nByteRead,i,buf[i]);
				fflush(stdout);
			}
			
			socks_mode = decode_socks_request();
			access = check_firewall();
			printf("access is %d\n",access);
			

			fd_set readfds;
			fd_set rs;
			fd_set writefds;
			fd_set ws;
			struct timeval timeout;
			struct sockaddr_in server_sin,server_sin1;
			struct hostent *he;
			struct sockaddr_in  cliSockaddr1 ;
			socklen_t nsockaddrsize1 ;
			//int server_fd,server_fd1,socks_fd,n,error;
			int nfds = FD_SETSIZE;

		

			timeout.tv_sec  = 0;
			timeout.tv_usec = 1;
			
			
			if(access){

					
				if(socks_mode == CONNECT){	
					// ---------setting reply package------------
					/*package[0] = VN;
					package[1] = 90;//request granted
					package[2] = DST_PORT/256;
					package[3] = DST_PORT%256;
					package[4] = DST_IP >> 24;
					package[5] = (DST_IP >> 16) & 0xFF;
					package[6] = (DST_IP >> 8) & 0xFF;
					package[7] = DST_IP & 0xFF;
				
					sprintf(ip_address,"%u.%u.%u.%u",package[4],package[5],package[6],package[7]);
					printf("ip is %s,port is %d\n",ip_address,DST_PORT);
					fflush(stdout);
				
					
					
					
					write(cliSock,package,package_size);
					*/
				
					// --------------connect to server--------------
					he = gethostbyname(mySock.dst_ip);
					server_fd = socket(AF_INET,SOCK_STREAM,0);
					bzero(&server_sin,sizeof(server_sin));
					server_sin.sin_family = AF_INET;
					server_sin.sin_addr = *((struct in_addr *)he->h_addr); 
					server_sin.sin_port = htons(mySock.dst_port);
					//fcntl(server_fd,F_SETFL, O_NONBLOCK);
					//fcntl(cliSock,F_SETFL, O_NONBLOCK);

					if(connect(server_fd,(struct sockaddr *)&server_sin,sizeof(server_sin)) < 0){
						//printf("connection wrong\n");
						if(errno != EINPROGRESS){
						printf("connection failed\n");
						fflush(stdout);
						return -1;
			
						}
					}

					printf("server_fd is %d\n",server_fd);	

				}

				
				
				if(socks_mode == BIND ){
					
					
					//struct sockaddr_in sa;
					int salen;
					int aa;
					//getsockname( sd, (struct sockaddr FAR *)&sa, &salen );

					bindflag=1;	
					
					he = gethostbyname(mySock.dst_ip);
					//server_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
					socks_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
					bzero(&server_sin1,sizeof(server_sin1));
					server_sin1.sin_family = AF_INET;
					server_sin1.sin_addr.s_addr = INADDR_ANY;
					server_sin1.sin_port = htons(0); ;
					

					if(bind(socks_fd, (struct sockaddr*)&server_sin1, sizeof(server_sin1))<0){
						printf("can't bind socket to port\n");
						exit(1);
					}
					
					salen = sizeof(server_sin1);
					getsockname( socks_fd, (struct sockaddr*)&server_sin1,&salen  );
					printf("port is %d\n",ntohs(server_sin1.sin_port));
					fflush(stdout);
			
					unsigned char package[8];
					package[0] = mySock.vn;
					package[1] = 90;//request granted
					package[2] = ntohs(server_sin1.sin_port)/256;
					package[3] = ntohs(server_sin1.sin_port)%256;
					package[4] = 140;
					package[5] = 114;
					package[6] = 78;
					package[7] = 62;

					write(cliSock,package,sizeof(package));



					

					if(listen(socks_fd, 1) <0)
						printf("listen fails\n");

					server_fd = accept(socks_fd, (struct sockaddr*)&cliSockaddr1, &nsockaddrsize1) ;
					write(cliSock,package,sizeof(package));	
					//fcntl(server_fd,F_SETFL, O_NONBLOCK);
					//fcntl(cliSock,F_SETFL, O_NONBLOCK);


					printf("socks_fd accepted!\n");
					fflush(stdout);

					//getsockname( server_fd, (struct sockaddr*)&sa, &salen );
					//printf("port is %d\n",sa.sin_port);


				}

				FD_ZERO(&readfds);
				FD_ZERO(&rs);
				FD_ZERO(&writefds);
				FD_ZERO(&ws);

				FD_SET(server_fd,&rs);
				FD_SET(server_fd,&ws);
				FD_SET(cliSock,&rs);
				FD_SET(cliSock,&ws);
				//sucks_status = F_CONNECTING;
				sucks_status = C_READING;

				//dup2(cliSock,1);
				//close(cliSock);

			}
			
			
			
			
			
			
			readfds = rs;
			writefds = ws;

			
			while(1){
				memcpy(&readfds,&rs,sizeof(readfds));
				memcpy(&writefds,&ws,sizeof(writefds));
				
				
				if(select(nfds,&readfds,&writefds,NULL,&timeout) < 0)
					exit(1);
				
				

				if(FD_ISSET(server_fd,&readfds)){
				
					n = recv_msg(server_fd,cliSock,1);
					printf("n is %d\n",n);
					fflush(stdout);
					//usleep(400000);
					if(n==0){
						close(server_fd);
						close(cliSock);
						exit(1);
					}
				
				}
				
				if(FD_ISSET(cliSock,&readfds)){
				
					
					n = recv_msg(cliSock,server_fd,0);
					//usleep(400000);
					printf("n is %d\n",n);
					fflush(stdout);
					if(n==0){
						close(server_fd);
						close(cliSock);
						exit(1);
					}
				}

			}

			
			
			
			close(cliSock);
			exit(1);	
			
			/*
			while(1) {
				FD_ZERO(&readfds);
				FD_SET(cliSock,&readfds);

				if (select(cliSock+1, &readfds, NULL, NULL, NULL) < 0) {
					printf("[ERR] select failed!\n") ;
					exit(1) ;
				}
				// get a message from the client 
				if (FD_ISSET(cliSock,&readfds))
				{
					cur_pos = (cur_pos+1) % MAX_PIPE_NUM ;
					memset(strEcho, '\0', strlen(strEcho)) ;
					if ( (nByteRead = recv(cliSock, strEcho, STR_MAX, 0)) == SOCKET_ERROR )
					{
						printf("[ERR] Recv from client fault.\n");
						return -1 ;
					}
					strEcho[nByteRead] = '\0' ;
					printf("nByteRead: %d, Recv message from client: %s", nByteRead, strEcho);
					if ( (nByteRead <= 3) && (strEcho[0] == '\r') ) {
						printf("Receive empty comment.\n");
						send_msg_to_cli(cliSock, "") ;
						continue ;
					} 

					if(decode_msg(cliSock, strEcho) == EXIT)
						exit(1);


				}
				fflush(stdout) ;
			}
			*/
		} else {
			close(cliSock) ;
		}
	}

	/* close up socket */
	printf("\n[016] Close service socket\n") ;
    	close(srvSock) ;
        
        /* give client a chance to properly shutdown */
        sleep(1) ;

	return 0 ;
}

/*
 *  * The signal handler function -- only gets called when a SIGCHLD
 *   * is received, ie when a child terminates
 *    */
void sig_chld(int signo) 
{
    int status, child_val;

    /* Wait for any child without blocking */
    if (waitpid(-1, &status, WNOHANG) < 0) 
    {
        /*
 *          * calling standard I/O functions like fprintf() in a 
 *                   * signal handler is not recommended, but probably OK 
 *                            * in toy programs like this one.
 *                                     */
        fprintf(stderr, "waitpid failed\n");
        return;
    }

    /*
 *      * We now have the info in 'status' and can manipulate it using
 *           * the macros in wait.h.
 *                */
    if (WIFEXITED(status))                /* did child exit normally? */
    {
        child_val = WEXITSTATUS(status); /* get child's exit status */
        printf("child's exited normally with status %d\n", child_val);
    }
}

int check_exit(int cliSock, char *str)
{
	
	if ((strcmp(str, "exit") == 0))
	{	
		printf("\n[015] Close client socket") ;
		close(cliSock) ;
		return TRUE ;
	} else {
		return FALSE ;
	}
}








