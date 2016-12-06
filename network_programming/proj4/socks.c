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


#define EMPTY		0
#define COMMAND		1
#define PIPE		2
#define	WRITE_TO_FILE	3
#define UNKNOWN		4

#define NORMAL		0
#define	PIPE_IN		1
#define	PIPE_OUT	2
#define FILE_FD		3
#define	SOCKET		4
#define PASS_ERR	5

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
	char user_id[32] ;
	char domain_name[32] ;

};

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
int firewall=0;
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
		/*
		if(server_or_client)
			printf("\npid=%u,read from server-----------------------status is %d,len is %d,str(buf) is %d,buf[0] = %d,read is:\n%s\n",getpid(),sucks_status,len,strlen(buf),buf[0],buf);
		else
			printf("\npid=%u,read from client-----------------------status is %d,len is %d,str(buf) is %d,buf[0] = %d,read is:\n%s\n",getpid(),sucks_status,len,strlen(buf),buf[0],buf);
		
		fflush(stdout);
		*/
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
	nRetValue = listen(srvSock, 10) ;
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
			

			//clientStruct new_cli;
			ip = inet_ntoa(cliSockaddr.sin_addr);

			sprintf(new_cli.ip,"%s",ip) ;
			new_cli.port = ntohs(cliSockaddr.sin_port) ;


			memset(buf,'\0',strlen(buf));
			
			nByteRead = recv(cliSock,buf,MAX_BUF,0);
			buf[nByteRead] = '\0';
			for(i=0;i<(nByteRead-1);i++){
				printf("read bytes %d,read message[%d]:%u\n",nByteRead,i,buf[i]);
				fflush(stdout);
			}
			

			//1.decode request-----------------------------
			int access;
			int mode;
			struct Sock4 mySock;
			int package_size = 8;
			unsigned char package[package_size];
			unsigned char package_to_ftp[30];
			unsigned char ip_address[30];
			struct sockaddr_in server_sin,server_sin1;
			struct hostent *he;
			struct sockaddr_in  cliSockaddr1 ;
			socklen_t nsockaddrsize1 ;
			//int server_fd,server_fd1,socks_fd,n,error;
			int nfds = FD_SETSIZE;
			struct timeval timeout;

			unsigned char VN = buf[0];
			unsigned char CD = buf[1];
			unsigned int DST_PORT = (buf[2] << 8) | buf[3];
			unsigned int DST_IP = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
			char * USER_ID = buf + 8;
			char * domain_name;
			
			//connect_mode or bind_mode -------------
			if(CD == 1)
				mode = 1;
			if(CD == 2)
				mode = 0;

			printf("mode(connect=1/bind=0) is %d\n",mode);
			fflush(stdout);

			
			printf("VN: %u\n",mySock.vn = VN);
			printf("CD: %u\n",CD);
			printf("D_PORT:%u\n",mySock.dst_port = DST_PORT);
			sprintf(mySock.dst_ip,"%u.%u.%u.%u",buf[4],buf[5],buf[6],buf[7]);
			printf("D_IP:%s\n",mySock.dst_ip);
			printf("S_PORT:%u\n",new_cli.port);
			printf("S_IP:%s\n",new_cli.ip);
			printf("UID:%s\n",USER_ID);
			
			if(strncmp(mySock.dst_ip, "0.0.0.", 6) == 0){
				struct hostent *hhe ;
				struct sockaddr_in local_addr ;
				struct in_addr dst_addr ;
				char *tmp = buf + 8 ;
				char *tmp1 ;

				tmp1 = strtok(tmp, NULL) ;
				strcpy(USER_ID, tmp1) ;

				tmp1 = strtok(NULL, NULL) ;
				strcpy(domain_name, tmp1) ;

				if((hhe=gethostbyname(tmp1)) == NULL )
				{
					fprintf(stderr, "[ERR] IP format error!\n");
					exit(1);
				}
				memcpy(&local_addr.sin_addr.s_addr, hhe->h_addr, 4) ;
				dst_addr.s_addr = local_addr.sin_addr.s_addr ;
				strcpy(mySock.dst_ip, inet_ntoa(dst_addr)) ;
				
			
			}


			
			int check_format = 1;
			if(check_format){
			
				if(VN != 4){
					printf("socks format wrong!!");
					fflush(stdout);
					exit(1);
				}
				if(CD != 1 && CD != 2){
					printf("socks format wrong!!");
					fflush(stdout);
					exit(1);
				
				}

			
			
			}


			if(firewall){
				
				FILE * fp = fopen("firewall.conf","r");
				char tmp_buffer[100];
				char *ptr;

				if(fp == NULL){
					printf("can't open firewall.conf");
					fflush(stdout);
					exit(1);
				
				}
				while(fgets(tmp_buffer,100,fp) != NULL){
					if(!strncmp("permit",tmp_buffer,6)){
						
						ptr = &tmp_buffer[7];
						if(ptr[0] != '*'){
							/*if(!strncmp(ptr,new_cli.ip,3)){
								access = 1;
								break;
								//rintf("SOCKS_CONNECT GRANTED....\n");
								//fflush(stdout);

							}
							else{
								access = 0;
								//printf("SOCKS_CONNECT REJECT....\n");
								//fflush(stdout);
								//exit(1);
							}*/

							if(strncmp(ptr,mySock.dst_ip,3)){
								access = 1;
								break;
							}
							else{
								access = 0;
							
							}
						}
						else{
							access = 1;
						}
					
					
					}
				}


				if(access){				
					printf("SOCKS_CONNECT GRANTED....\n");
					fflush(stdout);
				}else{
					printf("SOCKS_CONNECT REJECT....\n");
					fflush(stdout);
					                   package[0] = VN;
									   package[1] = 91;//request granted
									   package[2] = DST_PORT/256;
									   package[3] = DST_PORT%256;
									   package[4] = DST_IP >> 24;
									   package[5] = (DST_IP >> 16) & 0xFF;
									   package[6] = (DST_IP >> 8) & 0xFF;
									   package[7] = DST_IP & 0xFF;
									   write(cliSock,package,package_size);

					exit(1);
				}
				
			
			}else{
				access = 1;
				printf("SOCKS_CONNECT GRANTED....\n");

			
			}
			
			
			
			fd_set readfds;
			fd_set rs;
			fd_set writefds;
			fd_set ws;

			timeout.tv_sec  = 0;
			timeout.tv_usec = 1;

			//access = 1;

			if(access){

					
				if(mode ==1){	
					// ---------setting reply package------------
					package[0] = VN;
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
				
				
					// --------------connect to server--------------
					//he = gethostbyname(ip_address);
					he = gethostbyname(mySock.dst_ip);
					server_fd = socket(AF_INET,SOCK_STREAM,0);
					bzero(&server_sin,sizeof(server_sin));
					server_sin.sin_family = AF_INET;
					server_sin.sin_addr = *((struct in_addr *)he->h_addr); 
					server_sin.sin_port = htons(DST_PORT);
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

					if(DST_PORT == 21){
						
						//strcpy(buf,"PORT 7788");
						//write(server_fd,buf,strlen(buf));
					
					
					}
					printf("server_fd is %d\n",server_fd);	

				}

				
				
				if(mode==0){
					
					
					//struct sockaddr_in sa;
					int salen;
					int aa;
					//getsockname( sd, (struct sockaddr FAR *)&sa, &salen );

					bindflag=1;	
					
					//he = gethostbyname(ip_address);
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
			
					
					package[0] = VN;
					package[1] = 90;//request granted
					package[2] = ntohs(server_sin1.sin_port)/256;
					package[3] = ntohs(server_sin1.sin_port)%256;
					package[4] = 140;
					package[5] = 114;
					package[6] = 78;
					package[7] = 62;

					write(cliSock,package,sizeof(package));



					if(DST_PORT == 21){


					}		
					

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

int check_cmd(int cliSock, char *str)
{
	DIR *dir ;
	struct dirent *ptr ;
	int i, cnt=0 ;
	char resolved_path[64] ;
	char *path[16] ;

	env = getenv("PATH");
	env[strlen(env)] = '\0' ;
	path[cnt++] = strtok(env, ":\0") ;	
	while((path[cnt]=strtok( NULL, ":\0")) != NULL) {
		cnt++;
	}
		
        for(i=0; i<cnt; i++) {
		realpath(path[i], resolved_path) ;
		dir = opendir(resolved_path) ;
		while((ptr = readdir(dir)) != NULL)
		{
			if(strcmp(ptr->d_name, str) == 0) {
				closedir(dir) ;
				return FIND ;
			}
		}
	}
	closedir(dir) ;
	
	return UNFIND ;
}

int create_one_round_pipe(int cliSock, CMD_STRUCT *cmd_list, int cmd_list_num) 
{
	int i=0, fp=-1, status;
	int fd[cmd_list_num][2] ;
	int chi_pid ;
	printf("cmd_list_num is %d\n",cmd_list_num);
	for (i=0; i<cmd_list_num; i++) {
		if(i < (cmd_list_num-1)) {
			if (pipe(fd[i]) <0) {
				printf("can't create pipe\n") ;
				return -1 ;
			}
		} else if( i == (cmd_list_num-1) ){
			if(cmd_list[i].pipe_cross_msg == TRUE) {
				printf("pipe_table[cmd_list[i].pipe_cross_num][0] is %d\n",pipe_table[cmd_list[i].pipe_cross_num][0]);
				if(pipe_table[cmd_list[i].pipe_cross_num][0] == -1){
					if (pipe(pipe_table[cmd_list[i].pipe_cross_num]) <0) {
						printf("can't create pipe\n") ;
						return -1 ;
					}
				}
			}		
		}

		if ((chi_pid = fork()) < 0 ) {
			printf("can't fork\n") ;
			return -1 ;
		} else if (chi_pid == 0) {
			printf("i is %d\n",i);
			if(i == 0){ 
				//printf("pipe_table[cur_pos] is %d\n",pipe_table[cur_pos]);
				if(pipe_table[cur_pos] != NULL) {
					close(pipe_table[cur_pos][1]) ;
					dup2(pipe_table[cur_pos][0], 0) ;
					close(pipe_table[cur_pos][0]) ;
				}
			}
		  	if(i > 0) {
				close(fd[i-1][1]);
            	dup2(fd[i-1][0], 0);
                close(fd[i-1][0]);
			}

		    if(i < (cmd_list_num-1) ) {
				close(fd[i][0]);
                dup2(fd[i][1], 1);
				if(cmd_list[i].err == PIPE_OUT) {
                	dup2(fd[i][1], 2);
				} else
					dup2(cliSock, 2) ;
                close(fd[i][1]);
			}
		
			if(i == (cmd_list_num-1)) { // last command
				if(cmd_list[i].pipe_cross_msg == TRUE) { // |number
					close(pipe_table[cmd_list[i].pipe_cross_num][0]) ;
					dup2(pipe_table[cmd_list[i].pipe_cross_num][1], 1) ;
					if(cmd_list[i].err == PIPE_OUT) {
						dup2(pipe_table[cmd_list[i].pipe_cross_num][1], 2) ;
					} else
						dup2(cliSock, 2) ;
					close(pipe_table[cmd_list[i].pipe_cross_num][1]) ;
				} else if(cmd_list[i].out == FILE_FD) {
					fp = open(cmd_list[i].filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                                	if(fp == -1) {
                                        	printf("file %s open error\n", cmd_list[i].filename);
                                        	exit(1);
                                	}
					dup2(fp, 1);
					if(cmd_list[i].err == FILE_FD)
						dup2(fp, 2);
					else
						dup2(cliSock, 2) ;
					close(fp) ;
				} else {
					dup2(cliSock, 1) ;
                    dup2(cliSock, 2) ;
				}
			}
			close(cliSock) ;
			status = execvp( cmd_list[i].argv[0], cmd_list[i].argv ) ;
			exit(status) ;
		} else {	
			if(i == 0)
				if(pipe_table[cur_pos] != NULL) {
					close(pipe_table[cur_pos][1]) ;
					close(pipe_table[cur_pos][0]) ;
					free(pipe_table[cur_pos]) ;
					pipe_table[cur_pos] = NULL ;
				}

			if(i > 0) {
                                close(fd[i-1][1]);
                                close(fd[i-1][0]);
            }

            if(i < (cmd_list_num-1) ) {
				
            }

            if(i == (cmd_list_num-1)) {
				if(cmd_list[i].pipe_cross_msg == FALSE)
					waitpid(chi_pid, &status, 0);
			}
			wait(&status) ;
			fflush(stdout) ;
			
		}

	}
	send_msg_to_cli(cliSock, "\0") ;
}

int decode_msg(int cliSock, char *msg)
{
	int i=0, j=0, len=0, status=0 ; 
	int last_str_type = EMPTY, pipe_extend_round=0 ; 
	int /*cmd_list_num=1,*/ cmd_list_cnt=-1;
	char *ptr ;
	char buf[1024] ;
	char *input_argv[MAX_ARGV_NUM], *cmd_argv[MAX_ARGV_NUM] ;
	int input_argc=0, cmd_argc=0 ;
	
	//ptr = strtok( msg, " \n\r") ;
	//input_argv[input_argc] = (char *) malloc( strlen(ptr) * sizeof(char *) ) ;
	printf("%s\n",msg);
	input_argv[input_argc] = strtok( msg, " \n\r\t") ;
	input_argv[input_argc][strlen(input_argv[input_argc])] = '\0' ;
	if( (input_argv[input_argc][0] == '|') || (input_argv[input_argc][0] == '!') ) {
		cmd_list_num++ ;
		//pipe_num++ ;
	}
	//printf("argv[%d]: %s, length: %d\n", input_argc, input_argv[input_argc], strlen(input_argv[input_argc])) ;
	while( input_argv[++input_argc]=strtok( NULL, " \n\r\t") )
	{
		input_argv[input_argc][strlen(input_argv[input_argc])] = '\0' ;
		if( (input_argv[input_argc][0] == '|') || (input_argv[input_argc][0] == '!') ) {
                	cmd_list_num++ ;
			//pipe_num++ ;
		}
		//printf("argv[%d]: %s, length: \n", input_argc, input_argv[input_argc]) ;
	} 
	if((input_argv[input_argc-1][0] == '|') || (input_argv[input_argc-1][0] == '!'))
		cmd_list_num--;
	//printf("\n\nargc: %d\n", input_argc) ;

	if(check_exit(cliSock, input_argv[0]) == TRUE)
		return EXIT;

        //CMD_STRUCT cmd_list[cmd_list_num] ;
        cmd_list = (CMD_STRUCT *) malloc ( cmd_list_num * sizeof(CMD_STRUCT)) ;
	
	last_str_type = EMPTY ;
	printf("input_argv is %d\n",input_argc);
	for(i=0; i<input_argc; i++)
	{
		if ((input_argv[i][0] == '|') || (input_argv[i][0] == '!')) {
			if(last_str_type == COMMAND) {
				last_str_type = PIPE ;
				cmd_list_cnt++ ;
				printf("cmd_argc is %d\n",cmd_argc);
				cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(void *)) ;
                                for(j=0; j<cmd_argc; j++) {
                                        cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
                                        strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
                                }
				cmd_list[cmd_list_cnt].argv[j] = 0 ;
				cmd_argc = 0 ;
                cmd_argv[0] = 0 ;
				printf("cmd_list_cnt is %d\n",cmd_list_cnt);
				if(cmd_list_cnt==0)
					cmd_list[cmd_list_cnt].in = NORMAL ;
				
				cmd_list[cmd_list_cnt+1].in = PIPE_IN ;
				cmd_list[cmd_list_cnt].out = PIPE_OUT ;
				cmd_list[cmd_list_cnt].err = NORMAL ;
				
				if(input_argv[i][0] == '!') { 
					cmd_list[cmd_list_cnt].err = PIPE_OUT ;
				}

				if (input_argv[i][1] != '\0') {
					ptr = &input_argv[i][1] ;
					pipe_extend_round = atoi(ptr) ;
					if ((pipe_extend_round<1) ||(pipe_extend_round>1000)) {                         
						printf("[ERR]: pipe extend number is not resonable%d\n", pipe_extend_round) ;
						return -1 ;
					}	
					pipe_extend_round = (cur_pos+pipe_extend_round) % MAX_PIPE_NUM ;
					if(pipe_table[pipe_extend_round] == NULL ) {
						pipe_table[pipe_extend_round] = (int *)malloc(2*sizeof(int)) ;
						if(pipe_table[pipe_extend_round] == NULL){
							printf("malloc() error\n");
							exit(1);
						}
						pipe_table[pipe_extend_round][0] = -1 ;
						pipe_table[pipe_extend_round][1] = -1 ;
					}
					cmd_list[cmd_list_cnt].pipe_cross_msg = TRUE ;
					cmd_list[cmd_list_cnt].pipe_cross_num = pipe_extend_round ;
					
				} else 
					cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
			} else { 
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
				return -1 ; 
			}
		/*} else if (input_argv[i][0] == '!') {
			if(last_str_type == COMMAND) {
				if(pipe_cnt != -1)
					pipe_set[pipe_cnt].r_argv = cmd_argv ;
				last_str_type = PIPE ;
				pipe_cnt++ ;
				pipe_set[pipe_cnt].pass_stderr = TRUE ;
				pipe_set[pipe_cnt].write_to_file = FALSE ;
				if (input_argv[i][1] != '\0') {
					ptr = &input_argv[i][1] ;
					pipe_set[pipe_cnt].reserve_total_cnt = atoi(ptr) ;
					pipe_set[pipe_cnt].reserve_left_cnt = pipe_set[pipe_cnt].reserve_total_cnt ;
					if ((pipe_set[pipe_cnt].reserve_total_cnt<1) ||(pipe_set[pipe_cnt].reserve_total_cnt>1000)) {
						printf("[ERR]: pipe extend number is not resonable%d\n", pipe_set[pipe_cnt].reserve_total_cnt) ;
						return -1 ;
					}
				}
				pipe_set[pipe_cnt].l_argv = (char **)malloc(cmd_argc*sizeof(void *)) ;
                                for(j=0; j<cmd_argc; j++) {
                                        pipe_set[pipe_cnt].l_argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
                                        strcpy( pipe_set[pipe_cnt].l_argv[j], cmd_argv[j] );
                                }
                                pipe_set[pipe_cnt].l_argv[j] = 0 ;
				//pipe_set[pipe_cnt].l_argv = cmd_argv ;
				cmd_argc = 0 ;
				cmd_argv[0] = 0 ;
			} else {
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
                                return -1 ;
			}*/
		} else if (input_argv[i][0] == '>') {	
			if(last_str_type == COMMAND) {
				last_str_type = WRITE_TO_FILE ;

				cmd_list_cnt++ ;
                                cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(void *)) ;
                                for(j=0; j<cmd_argc; j++) {
                                        cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
                                        strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
                                }
				cmd_list[cmd_list_cnt].argv[j] = 0 ;
				cmd_argc = 0 ;
                                cmd_argv[0] = 0 ;

				if(cmd_list_cnt == 0)
                                	cmd_list[cmd_list_cnt].in = NORMAL ;
                                cmd_list[cmd_list_cnt].out = FILE_FD ;
				cmd_list[cmd_list_cnt].err = SOCKET ;
				cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
				if((i+1) < input_argc) {
					//cmd_list[cmd_list_cnt].filename = input_argv[i+1] ;
					strcpy(cmd_list[cmd_list_cnt].filename,  input_argv[i+1]) ;
					break ;
				}
			} else {
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
				return -1 ;
			}
		} else if (input_argv[i][0] == '<') {	
			if(last_str_type == COMMAND) {
				last_str_type = WRITE_TO_FILE ;

				cmd_list_cnt++ ;
				cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(void *)) ;
				for(j=0; j<cmd_argc; j++) {
					cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
					strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
				}
				cmd_list[cmd_list_cnt].argv[j] = 0 ;
				cmd_argc = 0 ;
				cmd_argv[0] = 0 ;

				if(cmd_list_cnt == 0) {
					cmd_list[cmd_list_cnt].in = NORMAL ;
					cmd_list[cmd_list_cnt].out = FILE_FD ;
					cmd_list[cmd_list_cnt].err = FILE_FD ;
					cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
					if((i+1) < input_argc) {
						//cmd_list[cmd_list_cnt].filename = input_argv[i+1] ;
						strcpy(cmd_list[cmd_list_cnt].filename,  input_argv[i+1]) ;
						break ;
					}
				} else {
					printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
					return -1 ;
				}
			}
		} else if (strcmp(input_argv[i], "setenv") == 0) {
			if( last_str_type == EMPTY ){
				if((i+2) < input_argc) {
					if(setenv(input_argv[i+1], input_argv[i+2], 1) == -1) {
						printf("Set failed\n") ;
                                                send_msg_to_cli(cliSock, "Setenv Failed!\n") ;
					}
					//env = getenv(input_argv[i+1]) ;
                                        //len = strlen(env) ;
                                        //env[len] = '\n' ;
                                        //env[len+1] = '\0' ;
					send_msg_to_cli(cliSock, "") ;
					return 0 ;
				}
			} else if ( last_str_type == PIPE ) { /* no this condition */	
			} else if ( last_str_type == WRITE_TO_FILE) {
				//cmd_list[cmd_list_cnt].filename = input_argv[i] ;	
			} else if( last_str_type == COMMAND ) {
				cmd_argv[cmd_argc++] = input_argv[i] ;
				cmd_argv[cmd_argc] = 0 ;
			} else { /* unknown command before */
			}
		} else if (strcmp(input_argv[i], "printenv") == 0) {
			if ( last_str_type == EMPTY ){
        	 		if(input_argc > (i+1) ) {
                                        //printf("env ori: %s\n", input_argv[i+1]) ;
                                        env = getenv(input_argv[i+1]);
                                        //len = strlen(env) ;
                                        //env[len] = '\n' ;
                                        //env[len+1] = '\0' ;
                                        //printf("env aft: %s\n", env) ;
                                        memset(buf, '\0', STR_MAX) ;
        				sprintf(buf, "PATH=%s\n", env);
                                        send_msg_to_cli(cliSock, buf) ;
					return 0;
                                }
			} else if ( last_str_type == COMMAND ) {
				cmd_argv[cmd_argc++] = input_argv[i] ;
				cmd_argv[cmd_argc] = 0 ;
				
        	} else if ( last_str_type == WRITE_TO_FILE) {
                                //cmd_list[cmd_list_cnt].filename = input_argv[i] ;
        	} else { /* unknown command before */
			}
        } else if (last_str_type == COMMAND) {
			cmd_argv[cmd_argc++] = input_argv[i] ;
                        cmd_argv[cmd_argc] = 0 ;
		} else if (last_str_type == WRITE_TO_FILE) {
			//cmd_list[cmd_list_cnt].filename = input_argv[i] ;
		} else {
			if( check_cmd(cliSock, input_argv[i]) == FIND ) {
				if( (last_str_type == PIPE) || (last_str_type == EMPTY)) {
					printf("command%d is %s\n",i,input_argv[i]);
					cmd_argv[0] = input_argv[i] ;
					cmd_argv[1] = 0 ;
					cmd_argc = 1 ;
					last_str_type = COMMAND ;
				}
			} else { /* unknown command */
				memset(buf, '\0', strlen(buf)) ;
                        	sprintf(buf, "Unknown command: [%s].\n", input_argv[i]);
                        	send_msg_to_cli(cliSock, buf) ;
				return 0 ;
			}
		} 
	}
	
	if (last_str_type == COMMAND) {
		cmd_list_cnt++ ;
		cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(void *)) ;
		for(j=0; j<cmd_argc; j++) {
			cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
			strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
		}
		cmd_list[cmd_list_cnt].argv[j] = 0 ;
		cmd_list[cmd_list_cnt].out = SOCKET ;
		cmd_list[cmd_list_cnt].err = SOCKET ;
		cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
	}


	create_one_round_pipe(cliSock, cmd_list, cmd_list_num) ;
	cmd_list = NULL ;		
	cmd_list_num = 1 ;

	return 0;
}


int send_msg_to_cli(int cliSock, char *msg)
{
	int nByteWrite ;
	char buf[STR_MAX], symbol[] = "% " ;

	memset(buf, '\0', STR_MAX) ;
	sprintf(buf, "%s%s", msg, symbol);
	if((nByteWrite = send(cliSock, buf, strlen(buf), 0)) == -1)
	{
		printf("[ERR] Send to client fault.\n");
		return -1;
	}
	printf("nByteWrite: %d, Send message to client: %s\n", nByteWrite, buf);
	//memset(buf, '\0', strlen(buf)) ;
	fflush(stdout);	
	
	return 0;
}

