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
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>


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

#define EMPTY		0
#define COMMAND		1
#define PIPE		2
#define	WRITE_TO_FILE	3
#define UNKNOWN		4
#define PIPE_FOR_OTHERS	5

#define NORMAL		0
#define	PIPE_IN		1
#define	PIPE_OUT	2
#define FILE_FD		3
#define	SOCKET		4
#define PASS_ERR	5

#define MAX_USER   10
#define SHMKEY ((key_t)7890)
#define SHMKEY1 ((key_t)7891)
#define SHMKEY2 ((key_t)7892)



struct cmd_struct {
	char **argv ;
	int in ;
	int out ;
	int err ;
	int only_err;
	int pipe_cross_msg ;
	int pipe_cross_num ;
	int pipe_for_others;
	char filename[64] ;
	//char *c;
	//int argc;
} ;

struct user_struct {
	int available;
	int id;
	char name[64];
	char ip[30];
	int port;
	int fd;
	int pipe_for_others;
	int cur_pos;
	char pipe_command[50];
	char env[50];
	int who_knock;
	char message[100];

};

struct message_buffer{
	//char in[100];
	int for_who;	
	char out[100];


};

typedef struct cmd_struct CMD_STRUCT ;

void sig_chld(int);     /* prototype for our SIGCHLD handler */
int send_msg_to_cli(int cliSock, char *msg) ;
int send_msg_to_other(int cliSock, char *msg) ;
int decode_msg(int cliSock, char *msg) ;
int check_cmd(int cliSock, char *str) ;
int check_exit(int cliSock, char *str) ;
int create_one_level_pipe(int cliSock, CMD_STRUCT *cmd_list, int cmd_list_num) ;
char *env;
int *pipe_table[MAX_PIPE_NUM] ;
//int *pipe_for_others[MAX_USER];
int *pipe_for_others;
int cur_pos = -1, cmd_list_num=1;
int cur_user = -1;
CMD_STRUCT *cmd_list ;
struct user_struct *user;
//struct user_struct user[MAX_USER];
//int fd2user;
int error_flag;
int shmid,shmid1;
char strEcho[STR_MAX] ;
char fifo[MAX_USER][64];
char path[100]="./fiiifo.fifo";
char buf[1024] ;
int err_flag;
char welcome[] = 
"\
****************************************\n\
** Welcome to the information server. **\n\
****************************************\n\
" ;

char symbol[] = "% ", exit_msg[] = "exit \n" ;
int nRetValue;

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
	//int nRetValue ;
	int nByteRead, nByteWrite ;
	//char strEcho[STR_MAX] ; /* used for incomming data, and
    //                                    outgoing data */
	fd_set readfds ;
	fd_set activefds;
    //char buf[1024] ;
	//char symbol[] = "% ", exit_msg[] = "exit \n" ;
	char *env ;
	int i=0 ; 
	char *temp ;
	char resolved_path[64] ;
	pid_t pid ;
    int status, fp ;
	int nfds,fd;
	int alen;
	char name[]="(No Name)";
	char *ip;
	alen = sizeof(cliSockaddr);
	int val;
	//int shmid;
	//struct user_struct *user;



	//struct user_struct user[MAX_USER]; 

	struct sigaction act;
	/* Assign sig_chld as our SIGCHLD handler */
	act.sa_handler = sig_chld;

	/* We don't want to block any other signals in this example */
	sigemptyset(&act.sa_mask);

	/*
	 * * We're only interested in children that have terminated, not ones
	 * * which have been stopped (eg user pressing control-Z at terminal)
	 * */
	act.sa_flags = SA_NOCLDSTOP;

	/*
	 * * Make these values effective. If we were writing a real 
	 * * application, we would probably save the old value instead of 
	 * * passing NULL.
	 * */
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
	//nRetValue = listen(srvSock, MAX_USER) ;
	nRetValue = listen(srvSock, MAX_USER) ;
	if (nRetValue == SOCKET_ERROR)
	{
		printf("\n[ERR] Fail to service socket listen\n") ;
		close(srvSock) ;
		return 0;
	}

	printf("\n[007] Service socket listening Completed") ;
	printf("\n[008] Wainting for client...") ;
	
	
	nfds = getdtablesize();	
	FD_ZERO(&readfds);
	FD_SET(srvSock,&activefds);

	
	
	//set path
	setenv("PATH","bin:.",1);
	realpath("./ras",resolved_path);
	chdir(resolved_path);


	if((shmid=shmget(SHMKEY,MAX_USER*sizeof(struct user_struct),0666|IPC_CREAT))<0)
		perror("share memory error!\n");
	if((user=(struct user_struct *)shmat(shmid,(char *)0,0))==NULL)
		perror("share memory pointer error!\n");
	
	
	if((shmid1=shmget(SHMKEY1,MAX_USER*2*sizeof(int*),0666|IPC_CREAT))<0)
		perror("share memory error!\n");
	if((pipe_for_others=(int *)shmat(shmid1,(char *)0,0))==NULL)
		perror("share memory pointer error!\n");
	
	for(i=0;i<MAX_USER;i++){
		user[i].available=0;
		user[i].pipe_for_others=EMPTY;
		user[i].who_knock = -1;
		user[i].id = -1;
		strcpy(user[i].name,name);
		memset(buf,'\0', STR_MAX);
		sprintf(buf,"./fifo%d.fifo",i);
		strcpy(fifo[i],buf);
		unlink(fifo[i]);
	}	

	//fcntl(cliSock,F_SETFL, O_NONBLOCK);
	signal(SIGCHLD, SIG_IGN);
	while(1) {
		/* Waiting for a incoming connection */
		cliSock=accept(srvSock, (struct sockaddr*)&cliSockaddr, &nsockaddrsize) ;
		//fcntl(cliSock,F_SETFL, O_NONBLOCK);

		if(cur_user > MAX_USER ){
				printf("too many people\n");
				fflush(stdout);
				continue;
		}
			
			
			
		
		printf("\ncliSock is %d\n",cliSock);
		for(i=0;i<MAX_USER;i++){
			if(user[i].available != 1 ){
				printf("i is %d\n",i);
				cur_user=i;
				break;
			}
		}	
		
		//available
		user[cur_user].available = 1;

		//id
		user[cur_user].id = cur_user+1;

		//fd
		user[cur_user].fd = cliSock;
			
		//ip
		ip = inet_ntoa(cliSockaddr.sin_addr);
		sprintf(user[cur_user].ip,"%s",ip);
			
		//port
		user[cur_user].port = ntohs(cliSockaddr.sin_port);

		//name
		sprintf(user[cur_user].name,"%s",name);

		//env
		sprintf(user[cur_user].env,"bin:.");
		//position
		user[cur_user].cur_pos = -1;

		printf("\n\n #%d*******Coming from port %d, ip %s ,",user[cur_user].id,user[cur_user].port,user[cur_user].ip);
		printf("fd is %d\n\n",user[cur_user].fd);
		fflush(stdout);
			
			
		//welcome message
		//sprintf(buf,"%s\n%s",welcome,symbol);
		sprintf(buf,"%s\n",welcome);
		nRetValue = send(cliSock,buf,strlen(buf),0);
		//memset(buf,'\0',strlen(buf));
			
			
		memset(buf,'\0', STR_MAX);
		sprintf(buf,"*** User '%s' entered from %s/%d. ***\n",user[cur_user].name,user[cur_user].ip,user[cur_user].port);
			
		printf("---------------cur_user are %d\n",cur_user);

		for(i=0;i<MAX_USER;i++){
			 if(user[i].available==1){
			 	//send_msg_to_cli(user[i].fd, buf);
				user[i].who_knock=cur_user;
				strcpy(user[i].message,buf);
			}
		}
			
		if (cliSock == SOCKET_ERROR )
		{
			printf("\n[ERR] Can't accept client connection\n") ;
			close(srvSock) ;
			return 0;
		}
		else
			printf("\n[009] Accept one TCP ECHO CLIENT") ;

		pid = fork();
		if(pid < 0){
			printf("fork() error\n");
			exit(1);
		}
		else if(pid == 0){ // child process
			
			int id = cur_user+1;
			close(srvSock);
			// set path
			setenv("PATH", "bin:.", 1) ;
			realpath("./ras", resolved_path) ;
			chdir(resolved_path) ;

			printf("[011] Waiting for client message") ;
			printf("*******\n");


			memset(buf, '\0', strlen(buf)) ;
			fcntl(cliSock,F_SETFL, O_NONBLOCK);
			while(1) {
				//FD_ZERO(&readfds);
				//FD_SET(cliSock,&readfds);

				//if (select(cliSock+1, &readfds, NULL, NULL, NULL) < 0) {
				//	printf("[ERR] select failed!\n") ;
				//	exit(1) ;
				//}
				
				//printf("!!!!!!!!!!!!!%d\n",cliSock);
				//sleep(300);

				
				//memset(buf, '\0', strlen(buf)) ;
				if(user[cur_user].who_knock != -1){
					//sprintf(buf,"%s\n",user[fd2user(cliSock,user)].message);
					printf("#############\n");
					fflush(stdout);
					if(user[cur_user].who_knock == cur_user)
						send_msg_to_cli(cliSock,user[cur_user].message);
					else
						send_msg_to_other(cliSock,user[cur_user].message);
					user[cur_user].who_knock = -1;
				}
				


				/* get a message from the client */
				//if (FD_ISSET(cliSock,&readfds))
				//{
					memset(strEcho, '\0', strlen(strEcho)) ;
					val = 1;
					//fcntl(cliSock,F_SETFL, O_NONBLOCK);
					//ioctlsocket( 1, FIONBIO, &val);
					//fcntl(cliSock, F_SETFL, O_NDELAY);
					//if ( (nByteRead = recv(cliSock, strEcho, STR_MAX,0)) < 0)
					//{
					//	printf("[ERR] Recv from client fault.\n");
					//		return -1 ;
					//}
					//fcntl(cliSock,F_SETFL, O_NONBLOCK);
				if(nByteRead = recv(cliSock, strEcho, STR_MAX,0) > 0){
				
						//usleep(2000000);
						strEcho[nByteRead-3] = '\0' ;
						memset(buf,'\0', STR_MAX);
						sprintf(buf,"%s",strEcho);
						printf("\n\n\nnByteRead: %d, Recv message from client: %s", nByteRead, buf);
						if ( (nByteRead <= 3) && (strEcho[0] == '\r') ) {
							printf("Receive empty comment.\n");
							send_msg_to_cli(cliSock, "") ;
							continue ;
						} 
						printf("--id is %d --cliSock is %d\n",id,cliSock);
						user[cur_user].cur_pos = (user[cur_user].cur_pos+1) % MAX_PIPE_NUM ;
						
						if(decode_msg(cliSock, buf) == EXIT){
							user[cur_user].available = 0;
							printf("#%d %s is leaving..........\n",user[cur_user].id,user[cur_user].name);
							fflush(stdout);
							
							memset(buf,'\0', STR_MAX);
							sprintf(buf,"*** User '%s' left. ***\n",user[cur_user].name);
							for(i=0;i<MAX_USER;i++){
								if(user[i].available == 1)
								{
									printf("i is %d\n",i);
									user[i].who_knock=cur_user;
									strcpy(user[i].message,buf);
								}
									
							
							}
							
							//sleep(500);
							close(cliSock);
							exit(1);
							//return 0;
							//break;
						
						}
					//}

				}
				fflush(stdout) ;
				//}
			}
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

int fd2user(int fd,struct user_struct ptr[])
{	
	int i;
	for(i=0;i<MAX_USER;i++){
		if(ptr[i].id == fd){
			return i;
		}
	}
			
}

int id2user(int id,struct user_struct ptr[])
{
	int i;
	if(id>0 && ptr[id-1].available==1)
		return id-1;
	else
		return -1;
}
int check_exit(int cliSock, char *str)
{
	
	if ((strcmp(str, "exit") == 0))
	{	
		printf("\n[015] Close client socket\n") ;
		//close(cliSock) ;
		return TRUE ;
	} else {
		return FALSE ;
	}
}

int check_logout(int cliSock,char *str){

	printf("ffffffff\n");
	fflush(stdout);
	if ((strncmp(str, "exit",4) == 0))
	{

		printf("\n[17] logout\n");
		//cur_user--;
		//close(cliSock);
		//FD_CLR(cliSock,&activefds);
		return 1;
	}
	return 0;
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


int create_one_round_pipe(int cliSock, CMD_STRUCT *cmd_list, int cmd_list_num, int pipe_from_user) 
{
	int i=0, fp=-1, status,j,k;
	int fd[cmd_list_num][2] ;
	pid_t chi_pid ;
	char *ptr;
	int user_id;
	int not_print_symbol_flag = 0;
	int myfifo;
	printf("\ncmd_list_num is %d,pipe_from_user is %d ------------------------\n\n",cmd_list_num,pipe_from_user);

	for (i=0; i<cmd_list_num; i++) {
		if(i < (cmd_list_num-1)) {
			if (pipe(fd[i]) <0) {
				printf("can't create pipe\n") ;
				return -1 ;
			}
		} else if( i == (cmd_list_num-1) ){
			if(cmd_list[i].pipe_cross_msg == TRUE) {
				if(pipe_table[cmd_list[i].pipe_cross_num][0] == -1)
					if (pipe(pipe_table[cmd_list[i].pipe_cross_num]) <0) {
						printf("can't create pipe\n") ;
						return -1 ;
					}
			}
			if(cmd_list[i].pipe_for_others == PIPE_FOR_OTHERS){
		
				
				unlink(fifo[cur_user]) ;
				if(mkfifo(fifo[cur_user], S_IRUSR| S_IWUSR|S_IFIFO|O_WRONLY ) == -1){
				//	fprintf(stderr, "*** Error: your pipe already exists. ***\n");
				//	fflush(stderr);
					//memset(buf, '\0', STR_MAX) ;
					sprintf(buf, "*** Error: your pipe already exists. ***") ;
					//send_msg_to_cli(cliSock, buf) ;
				//	exit(-1) ;
				}

				myfifo = open(fifo[cur_user],O_RDWR);
				
				//myfifo = open(fifo[cur_user], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
				if(myfifo < 0){
				
				
					printf("error fifo\n");
				
				
				}
			
			}
		}

		if ((chi_pid = fork()) < 0 ) {
			printf("can't fork\n") ;
			return -1 ;
		} else if (chi_pid == 0) {    
			if(i == 0){
				fflush(stdout);
				if(pipe_table[user[cur_user].cur_pos] != NULL) {
					close(pipe_table[user[cur_user].cur_pos][1]) ;
					dup2(pipe_table[user[cur_user].cur_pos][0], 0) ;
					close(pipe_table[user[cur_user].cur_pos][0]) ;
				}
			}
		  	
			if(pipe_from_user >= 0){
				printf("pipe out!!!!!!!!!\n");
				fflush(stdout);
				printf("fifo path is %s\n",fifo[pipe_from_user]);
				myfifo = open(fifo[pipe_from_user], O_RDONLY | O_NONBLOCK) ;
				if(myfifo < 0 || (read(myfifo, buf, sizeof*(buf))==EOF))
				{
				
					printf("error\n");
				
				}
				
				
				
				dup2(myfifo,0);
				close(myfifo);
			}
			
			if(i > 0) {
				close(fd[i-1][1]);
                dup2(fd[i-1][0], 0);
                close(fd[i-1][0]);
			}

		    if(i < (cmd_list_num-1) ) {
				if(cmd_list[i].only_err == PIPE_OUT){
				//if(err_flag ==1){
					close(fd[i][0]);
					dup2(fd[i][1], 2);
					close(fd[i][1]);
					printf("hahaha\n");
					fflush(stdout);
				}
				else{
					close(fd[i][0]);
                	dup2(fd[i][1], 1);
					if(cmd_list[i].err == PIPE_OUT) {
                   		dup2(fd[i][1], 2);
					} else
						dup2(cliSock, 2) ;
            		close(fd[i][1]);
				}
			}
		
			if(i == (cmd_list_num-1)) {
				if(cmd_list[i].pipe_cross_msg == TRUE) {
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
				}
				else if(cmd_list[i].pipe_for_others == PIPE_FOR_OTHERS){
					printf("waiting for others to take the pipe\n");
					
					dup2(myfifo, 1) ;
					if(cmd_list[i].err == 1) {
						dup2(myfifo, 2) ;
					} else
						dup2(cliSock, 2) ;
					//close(myfifo) ;
				}
				
				
				else {
					dup2(cliSock, 1) ;
                    dup2(cliSock, 2) ;
				}
			}
			close(cliSock) ;

			//if(error_flag==1)
			//	exit(0);
			status = execvp( cmd_list[i].argv[0], cmd_list[i].argv ) ;
			free(cmd_list[i].argv[0]);
			exit(status) ;
		} else {	
			if(i == 0){
				if(pipe_table[user[cur_user].cur_pos] != NULL) {
					close(pipe_table[user[cur_user].cur_pos][1]) ;
					close(pipe_table[user[cur_user].cur_pos][0]) ;
					free(pipe_table[user[cur_user].cur_pos]) ;
					pipe_table[user[cur_user].cur_pos] = NULL ;
				}
				
			}
			
			
			if(pipe_from_user >= 0){
				not_print_symbol_flag = 1;	

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

			if(cmd_list[i].pipe_for_others == PIPE_FOR_OTHERS){
				not_print_symbol_flag = 1;
				//unlink(fifo[pipe_from_user]) ;

			}
			
			//for(j=0;j<cmd_list[i].argc;j++){
			//	printf("freeee argc=%d, i=%d\n",cmd_list[i].argc,j);
			//	fflush(stdout);
			//	free(cmd_list[i].argv[j]);
			//	printf("freeee argc=%d, i=%d done!\n",cmd_list[i].argc,j);
			//}
			//free(cmd_list[i].argv);

			wait(&status) ;
			fflush(stdout) ;
			
		}

	}


	
	if(not_print_symbol_flag != 1)
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
	char whole_command_string[100];
	int	pipe_from_user= -1; 
	struct user_struct *user1;	
	
	error_flag=0;	
	
	sprintf(whole_command_string,"%s",msg);
	whole_command_string[strlen(msg)-2] = '\0';
	
	//ptr = strtok( msg, " \n\r") ;
	//input_argv[input_argc] = (char *) malloc( strlen(ptr) * sizeof(char *) ) ;
	input_argv[input_argc] = strtok( msg, " \n\r\t") ;
	input_argv[input_argc][strlen(input_argv[input_argc])] = '\0' ;
	if( (input_argv[input_argc][0] == '|') || (input_argv[input_argc][0] == '&') ) {
		cmd_list_num++ ;
		//pipe_num++ ;
	}
	//printf("argv[%d]: %s, length: %d\n", input_argc, input_argv[input_argc], strlen(input_argv[input_argc])) ;
	while( input_argv[++input_argc]=strtok( NULL, " \n\r\t") )
	{
		input_argv[input_argc][strlen(input_argv[input_argc])] = '\0' ;
		if( (input_argv[input_argc][0] == '|') || (input_argv[input_argc][0] == '&') ) {
                	cmd_list_num++ ;
			//pipe_num++ ;
		}
		//printf("argv[%d]: %s, length: \n", input_argc, input_argv[input_argc]) ;
	} 
	if((input_argv[input_argc-1][0] == '|') || (input_argv[input_argc-1][0] == '&'))
		cmd_list_num--;
	//printf("\n\nargc: %d\n", input_argc) ;

	if(check_exit(cliSock, input_argv[0]) == TRUE)
		return EXIT;
	
	//if(check_logout(cliSock, input_argv[0]) == 1)
	//	return 0;
        //CMD_STRUCT cmd_list[cmd_list_num] ;
        cmd_list = (CMD_STRUCT *) malloc ( cmd_list_num * sizeof(CMD_STRUCT)) ;
	
	last_str_type = EMPTY ;
	printf("\ninput_argc is %d\n",input_argc);
	for(i=0; i<input_argc; i++)
	{
		if ((input_argv[i][0] == '|') || (input_argv[i][0] == '!') || ((input_argv[i][0] == '&'))) {
			
			//if(last_str_type == COMMAND) {
				last_str_type = PIPE ;
				cmd_list_cnt++ ;
				cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(char **)) ;
                for(j=0; j<cmd_argc; j++) {
                    cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
                	strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
                }
				//cmd_list[cmd_list_cnt].argc = cmd_argc ;

				cmd_list[cmd_list_cnt].argv[j] = 0 ;
				cmd_argc = 0 ;
                                cmd_argv[0] = 0 ;
	
				if(cmd_list_cnt==0)
					cmd_list[cmd_list_cnt].in = NORMAL ;
				
				cmd_list[cmd_list_cnt+1].in = PIPE_IN ;
				cmd_list[cmd_list_cnt].out = PIPE_OUT ;
				cmd_list[cmd_list_cnt].err = NORMAL ;
				//cmd_list[cmd_list_cnt].argc = cmd_argc ;
				
				if(input_argv[i][0] == '!') { 
					cmd_list[cmd_list_cnt].err = PIPE_OUT ;
				}
				if(input_argv[i][0] == '&'){
					err_flag = 1;
					cmd_list[cmd_list_cnt].only_err = PIPE_OUT ;
					printf("&&&&\n");
				}
				if (input_argv[i][1] != '\0') {
					printf("extend pipe\n");
					ptr = &input_argv[i][1] ;
					pipe_extend_round = atoi(ptr) ;
					if ((pipe_extend_round<1) ||(pipe_extend_round>1000)) {                         
						printf("[ERR]: pipe extend number is not resonable%d\n", pipe_extend_round) ;
						return -1 ;
					}	
					//pipe_extend_round = (cur_pos+pipe_extend_round) % MAX_PIPE_NUM ;
					pipe_extend_round = (user[cur_user].cur_pos+pipe_extend_round) % MAX_PIPE_NUM ;
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
			//} else { 
			//	printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
			//	return -1 ; 
			//}
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
		} 
			
		else if(strcmp(input_argv[i], ">|") == 0){
			int index;
			//char path[100]="./fifo.tmp";

			/*
			if(mkfifo(path, S_IRWXU | S_IRWXO | S_IRWXG)==-1)
			{
				fprintf(stderr, "fail fifo\n"); fflush(stderr);
			}

			fifo[cur_user] = open(path,O_RDWR);
			close(1);
			dup(fifo[cur_user]);
			*/

			
			if(user[cur_user].pipe_for_others!=EMPTY){
				memset(buf,'\0', STR_MAX);
				sprintf(buf,"*** Error: your pipe already exists. **\n");
				send_msg_to_cli(cliSock, buf);
				return 0;
			
			}
			
			/*
			if(user[cur_user].pipe_for_others==EMPTY && pipe_for_others[cur_user] != -1){
					//pipe_for_others[fd2user(cliSock,user)] = (int *)malloc(2*sizeof(int));
					//if(pipe_for_others[fd2user(cliSock,user)]==NULL){
					//	printf("malloc() error\n");
					//	exit(1);
					//}
					pipe_for_others[2*cur_user] = -1;
					//pipe_for_others[fd2user(cliSock,user)][0] = -1;
					pipe_for_others[2*cur_user+1] = -1;
					//pipe_for_others[fd2user(cliSock,user)][1] = -1;
			}
			*/
			if(user[cur_user].pipe_for_others==EMPTY && last_str_type == COMMAND) {
			 	
				 memset(buf,'\0', STR_MAX);
				 sprintf(buf,"*** %s (#%d) just piped '%s' into his/her pipe. ***\n",user[cur_user].name,cur_user+1,whole_command_string);
				 sprintf(user[cur_user].pipe_command,"%s",whole_command_string);
				 
				 //for(index=0;index <= cur_user;index++)
				 //	send_msg_to_cli(user[index].fd, buf);
				for(index=0;index <MAX_USER;index++){
					if(user[index].available){
						user[index].who_knock=cur_user;
						strcpy(user[index].message,buf);
					
					}
				}
					
				last_str_type = PIPE ;
			    cmd_list_cnt++ ;
			    cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(void *)) ;
			    for(j=0; j<cmd_argc; j++) {
			    	cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
			        strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
			    }
			    cmd_list[cmd_list_cnt].argv[j] = 0 ;
			    cmd_argc = 0 ;
			    cmd_argv[0] = 0 ;
			 
			    if(cmd_list_cnt==0)
			    	cmd_list[cmd_list_cnt].in = NORMAL ;
			 
			    cmd_list[cmd_list_cnt+1].in = PIPE_IN ;
			    cmd_list[cmd_list_cnt].out = PIPE_OUT ;
			    cmd_list[cmd_list_cnt].err = NORMAL ;
				cmd_list[cmd_list_cnt].pipe_for_others = PIPE_FOR_OTHERS;
				user[cur_user].pipe_for_others = PIPE_FOR_OTHERS;
			 
			    if(input_argv[i][0] == '!') {
			    	cmd_list[cmd_list_cnt].err = PIPE_OUT ;
			    }
			
				
			} else {
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
			    return -1 ;
			}




		
			//return 0;	
		}
		
		else if(input_argv[i][0] == '<' && input_argv[i][1] != '\0'){
			//cmd_list_cnt++;
			
			int index;
			ptr = &input_argv[i][1];
			pipe_from_user = atoi(ptr)-1;
			

			if(user[pipe_from_user].pipe_for_others != PIPE_FOR_OTHERS){
				memset(buf,'\0', STR_MAX);
				sprintf(buf,"*** Error: the pipe from #%d does not exist yet. ***\n",pipe_from_user+1);
				send_msg_to_cli(cliSock, buf);
				return 0;
			}

				
			memset(buf,'\0', STR_MAX);
			sprintf(buf,"*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n",user[cur_user].name,user[cur_user].id,user[pipe_from_user].name,user[pipe_from_user].id,user[pipe_from_user].pipe_command);
			
			//for(index=0;index<=cur_user;index++){
			//	send_msg_to_cli(user[index].fd, buf);

			//}

			 for(index=0;index<MAX_USER;index++){
			 	if(user[index].available){
					user[index].who_knock =cur_user;
					strcpy(user[index].message,buf);
				}
			 }


			//last_str_type = PIPE;
			
			user[pipe_from_user].pipe_for_others = EMPTY;
			//cmd_list[cmd_list_cnt].pipe_for_others = EMPTY;	
		}
		
		else if (input_argv[i][0] == '>') {	
			if(last_str_type == COMMAND) {
				last_str_type = WRITE_TO_FILE ;

				cmd_list_cnt++ ;
                cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(char **)) ;
                for(j=0; j<cmd_argc; j++) {
                	cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
                    strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
                }
				cmd_list[cmd_list_cnt].argv[j] = 0 ;
				//cmd_list[cmd_list_cnt].argc = cmd_argc;
				cmd_argc = 0 ;
                cmd_argv[0] = 0 ;

				if(cmd_list_cnt == 0)
                   	cmd_list[cmd_list_cnt].in = NORMAL ;
                cmd_list[cmd_list_cnt].out = FILE_FD ;
				cmd_list[cmd_list_cnt].err = SOCKET ;
				cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
				//cmd_list[cmd_list_cnt].argc = cmd_argc;
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
					
					sprintf(user[cur_user].env,"%s",input_argv[i+2]);
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
        } 
		else if(strcmp(input_argv[i], "who") == 0){
			int index;
			int j;
			char title[] = "<ID>\t<nickname>\t<IP/port>\t<indicate me>";
			char temp[64];
			memset(buf,'\0', STR_MAX) ;
			memset(temp,'\0',sizeof(temp));
			
			sprintf(buf,"%s\n",title);
			for( index=0; index <MAX_USER; index++){
				if(user[index].available){
					if(user[index].id == cur_user+1)
						sprintf(temp,"%d\t%s\t%s/%d\t<-me\n",index+1,user[index].name,user[index].ip,user[index].port);
					else
						sprintf(temp,"%d\t%s\t%s/%d\n",index+1,user[index].name,user[index].ip,user[index].port);
					
					strcat(buf,temp);
					
					//for(j=0;j<(16-strlen(user[index].name));j++)
					//	temp[j]=' ';
					//temp[j]='\0';
					//strcat(buf,temp);
					
					//sprintf(temp,"%s/%d             ",user[index].ip,user[index].port);
					//strcat(buf,temp);
					//if(user[index].fd == cliSock){
					//	sprintf(temp,"%s","<- me");
					//	strcat(buf,temp);
					//}
					//sprintf(temp,"%s","\n");
					//strcat(buf,temp);
				}	
			}
			send_msg_to_cli(cliSock, buf) ;
			return 0;

		}
		else if(strcmp(input_argv[i], "tell") == 0){
			
			int index;
			char temp[100];
			memset(buf,'\0', STR_MAX);
			memset(temp,'\0', 100);

			for(index=2;index<input_argc;index++){
				strcat(temp,input_argv[index]);
				strcat(temp," ");

			}
			
			sprintf(buf,"*** %s told you ***:   %s\n",user[cur_user].name,temp);
			
			index=id2user(atoi(input_argv[i+1]),user);
			printf("index is %d\n",index);
			if(index < 0){
				memset(buf,'\0', STR_MAX);
				sprintf(buf,"*** Error: user #%d does not exist yet. ***\n",atoi(input_argv[i+1]));
				send_msg_to_cli(cliSock, buf);
				//user[cur_user].who_knock =1;
				//strcpy(user[cur_user].message,buf);
			}else{
				//send_msg_to_cli(user[index].fd, buf);
				user[index].who_knock = cur_user;
				strcpy(user[index].message,buf);
				
				if(cur_user != (user[index].id-1))
				 	send_msg_to_cli(cliSock, "");
			
			}
			
			//if(cliSock != user[index].fd)
			//	send_msg_to_cli(cliSock, "");
			return 0;
		
		}
		else if(strcmp(input_argv[i], "yell") == 0){
			int index;	
			char temp[50];
			memset(temp,'\0', 50);
			for(index=1;index<input_argc;index++){
				strcat(temp,input_argv[index]);
				strcat(temp," ");
			}
			memset(buf,'\0', STR_MAX);	
			sprintf(buf,"*** %s yelled ***: %s\n",user[cur_user].name,temp);
			//for(index=0;index <= cur_user;index++)
			//	send_msg_to_cli(user[index].fd, buf);
			
			
			//if((shmid=shmget(SHMKEY,MAX_USER*sizeof(struct user_struct),0666|IPC_CREAT))<0)
			//	perror("share memory error!\n");
			//user=(struct user_struct *)shmat(shmid,(char *)0,0);

			for(index=0;index<MAX_USER;index++){
			    if(user[index].available){
					user[index].who_knock=cur_user;
					strcpy(user[index].message,buf);
				}
			}

			
			return 0;
		
		
		}
		else if(strcmp(input_argv[i], "name") == 0){
			int index;
			sprintf(user[cur_user].name,"%s",input_argv[i+1]);

			memset(buf,'\0', STR_MAX);
			sprintf(buf,"*** User from %s/%d is named '%s'. ***\n",user[cur_user].ip,user[cur_user].port,user[cur_user].name);

			//for(index=0;index<=cur_user;index++)
			//	send_msg_to_cli(user[index].fd, buf);
			//
			for(index=0;index<MAX_USER;index++){
			     if(user[index].available){
				 	user[index].who_knock=cur_user;
					strcpy(user[index].message,buf);
				 
				 
				 }
			}


			return 0;
		
		}
		else if (last_str_type == COMMAND) {
			cmd_argv[cmd_argc++] = input_argv[i] ;
            cmd_argv[cmd_argc] = 0 ;
			
			if(input_argv[i][0] == '<' && input_argv[i][1] != '\0'){
				ptr = &input_argv[i][1];
				pipe_from_user = atoi(ptr)-1;
				if(user[pipe_from_user].pipe_for_others != PIPE_FOR_OTHERS){
					memset(buf,'\0', STR_MAX);
					sprintf(buf,"*** Error: the pipe from #%d does not exist yet. ***",pipe_from_user = atoi(ptr)+1);
					send_msg_to_cli(cliSock, buf);
					return 0;
				}
			}

		} else if (last_str_type == WRITE_TO_FILE) {
			//cmd_list[cmd_list_cnt].filename = input_argv[i] ;
		} else { 
			if( check_cmd(cliSock, input_argv[i]) == FIND ) {
				//if(error_flag!=1){
					if( (last_str_type == PIPE) || (last_str_type == EMPTY)) {
						cmd_argv[0] = input_argv[i] ;
						cmd_argv[1] = 0;
						cmd_argc = 1 ;
						last_str_type = COMMAND ;
					}
				//}
			} else { /* unknown command */
				memset(buf, '\0', strlen(buf)) ;
        		sprintf(buf, "Unknown command: [%s].\n", input_argv[i]);
                //send_msg_to_cli(cliSock, buf) ;
                send_msg_to_other(cliSock, buf) ;
				last_str_type = COMMAND ;
				//free(cmd_list);
				error_flag = 1;
				//break;
				//continue;
				//return 0 ;
			}
		} 
	}
	
	// for last command
	if (last_str_type == COMMAND) {
		printf("last command\n");
		cmd_list_cnt++ ;
		cmd_list[cmd_list_cnt].argv = (char **)malloc(cmd_argc*sizeof(char **)) ;
		for(j=0; j<cmd_argc; j++) {
			cmd_list[cmd_list_cnt].argv[j] = (char *)malloc(strlen(cmd_argv[j])*sizeof(char *)) ;
			strcpy( cmd_list[cmd_list_cnt].argv[j], cmd_argv[j] );
		}
		cmd_list[cmd_list_cnt].argv[j] = 0 ;
		cmd_list[cmd_list_cnt].out = SOCKET ;
		cmd_list[cmd_list_cnt].err = SOCKET ;
		cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
		cmd_list[cmd_list_cnt].pipe_for_others = EMPTY;
		//cmd_list[cmd_list_cnt].argc = cmd_argc;
	}

	//printf("cmd_list_num %d,cmd_list_cnt %d\n",cmd_list_num,cmd_list_cnt);	
	printf("error_flag is %d\n",error_flag);
	if(error_flag!=1)
		create_one_round_pipe(cliSock, cmd_list, cmd_list_num,pipe_from_user);
	else
		send_msg_to_cli(cliSock, "") ;

	cmd_list = NULL ;		
	cmd_list_num = 1 ;
	//free(cmd_list);	
	
	

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
		printf("[ERR] Send to client fd is %d fault.\n",cliSock);
		return -1;
	}
	printf("nByteWrite: %d, Send message to client: %s\n", nByteWrite, buf);
	//memset(buf, '\0', strlen(buf)) ;
	fflush(stdout);	
	
	return 0;
}

int send_msg_to_other(int cliSock, char *msg)
{
	int nByteWrite ;
	char buf[STR_MAX], symbol[] = "% " ;
	memset(buf, '\0', STR_MAX) ;
	sprintf(buf, "%s", msg);
	if((nByteWrite = send(cliSock, buf, strlen(buf), 0)) == -1)
	{
		printf("[ERR] Send to client fd is %d fault.\n",cliSock);
		return -1;
	
	}

	printf("nByteWrite: %d, Send message to client: %s\n", nByteWrite, buf);
	fflush(stdout);
	return 0;

}


int readline(int fd,char *ptr,int maxlen)
{
    int n,rc;
	char c;
		    
	for(n=1;n<maxlen;n++){
						
		if((rc=read(fd,&c,1)==1)){
	   		*ptr++ = c;
	  		printf("char is %c\n",c);
		 	if(c=='\n')
		    	break;
		}else if(rc==0){
																
			if(n==1)
				return 0;
			else
				break;
											
		}else
	   		return -1;
															         
	}
	return n;
}		





void reaper()
{
	union wait status;
	while(wait3(&status,WNOHANG,(struct rusage *)0)>0)
	;


}

