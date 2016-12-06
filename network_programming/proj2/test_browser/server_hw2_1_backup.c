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
#define EMPTY 		0
#define EXIST		1
#define MAX_CLIENT_NUM	30
#define MAX_ENV_NUM	32
#define NO_ERR		0
#define ERR		-1

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

struct cmd_struct {
	//char **argv ;
	int in ;
	int out ;
	int err ;
	int pipe_cross_msg ;
	int pipe_cross_num ;
	int pipe_from_user ;
	int pipe_to_user ;
	char filename[64] ;
	char **argv ;
} ;
typedef struct cmd_struct CMD_STRUCT ;

struct env_struct {
	char name[32] ;
	char path[STR_MAX] ;
} ;
typedef struct env_struct ENV_STRUCT ;

struct client_struct {
	char name[32] ;
 	char ip_addr[32] ;
	char rec_data[STR_MAX] ;
	int port ;
	int sock ;
	int id ;
	int cur_pos ;
	int *pipe_table[MAX_PIPE_NUM] ;
	int cur_env_num ;
	ENV_STRUCT env[MAX_ENV_NUM] ;
} ;
typedef struct client_struct CLIENT_STRUCT ;

void sig_chld(int);     /* prototype for our SIGCHLD handler */
int send_msg_to_cli(int cliSock, char *msg) ;
int decode_msg(int cliSock, char *msg) ;
int check_cmd(int cliSock, char *str) ;
int check_exit(int cliSock, char *str) ;
int create_one_level_pipe(int cliSock, CMD_STRUCT *cmd_list, int cmd_list_num) ;
int find_min_empty_cli() ;
int find_cli_fd_num(int sock) ;
int set_env(char *env_name, char *env_content) ;
char *get_env(char *env_name) ;
void print_all_client() ;
void chat_1to1(int from, int to, char *msg) ;
void broadcast(char *msg) ;
void exchange_cli_id(int id1, int id2) ;
/*char *env;
int *pipe_table[MAX_PIPE_NUM] ;
int cur_pos = -1, cmd_list_num=1;
CMD_STRUCT *cmd_list ;*/
int cur_cli = 0 ;
CLIENT_STRUCT cli_set[MAX_CLIENT_NUM] ;
int cli_survival[MAX_CLIENT_NUM] ;
int min_empty_cli = 0;
int *user_pipe_table[MAX_CLIENT_NUM] ;

char welcome[] = 
"\
****************************************\n\
** Welcome to the information server. **\n\
****************************************\
" ;

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
	fd_set readfds ;
	fd_set actfds ;
	int fd, nfds ;
    	char buf[STR_MAX] ;
	char symbol[] = "% ", exit_msg[] = "exit \n" ;
	//char *env ;
	int i=0 ; 
	char *temp ;
	char resolved_path[64] ;
	pid_t pid ;
        int status, fp ;
	//CLIENT_STRUCT cli_set[MAX_CLIENT_NUM] ;
	//int cli_survival[MAX_CLIENT_NUM] ;
	//int min_empty_cli = 0 ;

	for(i=0; i<MAX_CLIENT_NUM; i++)
	{
		//cli_set[i].name = (char *)malloc(strlen("(no name)")*sizeof(char *)) ;
		memset(cli_set[i].name, '\0', sizeof(cli_set[i].name)) ;
		strcpy(cli_set[i].name, "(no name)\0") ;
		cli_set[i].id = i+1 ;
		cli_set[i].cur_pos = -1 ; 
		//cli_set[i].env[0].name = (char *)malloc(strlen("PATH")*sizeof(char *)) ;
		memset(cli_set[i].env[0].name, '\0', sizeof(cli_set[i].env[0].name)) ;
		strcpy(cli_set[i].env[0].name, "PATH\0") ;
		//cli_set[i].env[0].name = "PATH" ;
		//cli_set[i].env[0].path = (char *)malloc(strlen("bin:.")*sizeof(char *)) ;
		memset(cli_set[i].env[0].path, '\0', sizeof(cli_set[i].env[0].path)) ;
		strcpy(cli_set[i].env[0].path, "bin:.\0") ;
		cli_set[i].cur_env_num = 1 ;
		cli_survival[i] = EMPTY ;
	}

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

	// set path
	setenv("PATH", "bin:.", 1) ;
	realpath("./ras", resolved_path) ;
	chdir(resolved_path) ;

	nfds = getdtablesize();
	FD_ZERO(&actfds);
	FD_SET(srvSock,&actfds);

	printf("\n[007] Service socket listening Completed") ;
	printf("\n[008] Wainting for client...") ;
	while(1) {
		memcpy(&readfds, &actfds, sizeof(readfds)) ;
		if (select(nfds, &readfds, NULL, NULL, NULL) < 0) {
			printf("[ERR] select failed!\n") ;
			exit(1) ;
		}

		/* Getting client connection*/
		if (FD_ISSET(srvSock,&readfds))
		{
			if(min_empty_cli == -1)
			{
				printf("too many clinets!\n") ;
			}
			else {
				cliSock=accept(srvSock, (struct sockaddr*)&cliSockaddr, &nsockaddrsize) ;
				if (cliSock == SOCKET_ERROR )
				{
					printf("\n[ERR] Can't accept client connection\n") ;
					close(srvSock) ;
					return 0;
				}
				else
					printf("\n[009] Accept one TCP ECHO CLIENT\n") ;
				FD_SET(cliSock, &actfds) ;
				min_empty_cli = find_min_empty_cli() ;
				cli_set[min_empty_cli].sock = cliSock ;
				//cli_set[min_empty_cli].port = ntohs(cliSockaddr.sin_port) ;
				cli_set[min_empty_cli].port = 5566 ;
				//strcpy(cli_set[min_empty_cli].ip_addr, inet_ntoa(cliSockaddr.sin_addr)) ;
				strcpy(cli_set[min_empty_cli].ip_addr, "nctu" ) ;
				printf("Hi there, from  %s\n", cli_set[min_empty_cli].ip_addr);
				printf("Coming from port %d\n\n", cli_set[min_empty_cli].port);
				cli_survival[min_empty_cli] = EXIST ;
				//min_empty_cli = find_min_empty_cli() ;
				memset(buf, '\0', strlen(buf)) ;
				//sprintf(buf, "%s\n%s", welcome, symbol);
				//nRetValue = send(cliSock, buf, strlen(buf), 0);
				sprintf(buf, "%s", welcome);
				send_msg_to_cli(cliSock, buf) ;
				memset(buf, '\0', strlen(buf)) ;
				sprintf(buf, "\n*** User '(no name)' entered from %s/%d. ***\n", cli_set[min_empty_cli].ip_addr, cli_set[min_empty_cli].port);
				broadcast(buf) ;
				memset(buf, '\0', strlen(buf)) ;
				sprintf(buf, "%s", symbol);
				send_msg_to_cli(cliSock, buf) ;
				//cli_survival[min_empty_cli] = EXIST ;
				//min_empty_cli = find_min_empty_cli() ;
				//memset(buf, '\0', strlen(buf)) ;

			}
		}

		/* Checking message receive from all clients */
		for(fd=0; fd<nfds; ++fd) 
		{
			if((fd != srvSock) && (FD_ISSET(fd, &readfds)))
			{
				cur_cli = find_cli_fd_num(fd) ;
				if(cur_cli == -1)
				{
					printf("[ERR] Received message failed!\n") ;
					exit(-1) ;
				}
				
				setenv("PATH", cli_set[cur_cli].env[0].path, 1) ;
				cli_set[cur_cli].cur_pos = (cli_set[cur_cli].cur_pos+1) % MAX_PIPE_NUM ;
				memset(strEcho, '\0', strlen(strEcho)) ;
				if ( (nByteRead = recv(fd, strEcho, STR_MAX, 0)) == SOCKET_ERROR )
				{
					printf("[ERR] Recv from client fault.\n");
					return -1 ;
				}
				strEcho[nByteRead] = '\0' ;
				printf("nByteRead: %d, Recv message from client: %s", nByteRead, strEcho);
				if ( (nByteRead <= 3) && (strEcho[0] == '\r') ) {
					printf("Receive empty comment.\n");
					send_msg_to_cli(fd, "") ;
					continue ;
				}

				if((decode_msg(fd, strEcho) == EXIT) || (nByteRead == 0)) {
					cli_survival[cur_cli] = EMPTY ;
					memset(buf, '\0', strlen(buf)) ;
					sprintf(buf, "\n*** User '%s' left. ***", cli_set[cur_cli].name);
					broadcast(buf) ;
					memset(cli_set[cur_cli].name, '\0', sizeof(cli_set[cur_cli].name)) ;
					strcpy(cli_set[cur_cli].name, "(no name)\0") ;
					cli_set[cur_cli].cur_pos = -1 ;
					memset(cli_set[cur_cli].env[0].path, '\0', sizeof(cli_set[cur_cli].env[0].path)) ;
					strcpy(cli_set[cur_cli].env[0].path, "bin:.") ;
					for(i=1;i <cli_set[cur_cli].cur_env_num; i++) {
						free(cli_set[cur_cli].env[i].name) ;
						free(cli_set[cur_cli].env[i].path) ;
					}
					cli_set[cur_cli].cur_env_num = 1 ;
					close(fd) ;
					FD_CLR(fd, &actfds) ;
					//break ;
				}
				else {
					memset(buf, '\0', STR_MAX) ;
					sprintf(buf, "\n%s", symbol);
					if((nByteWrite = send(fd, buf, strlen(buf), 0)) == -1)
					{
						printf("[ERR] Send to client fault.\n");
						return -1;
					}
				}
				fflush(stdout) ;
			}
		}
		fflush(stdout) ;
	}

	/* close up socket */
	printf("\n[016] Close service socket\n") ;
    	close(srvSock) ;
        
        /* give client a chance to properly shutdown */
        sleep(1) ;

	return 0 ;
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

int check_cmd(int cliSock, char *str)
{
	/*DIR *dir ;
	struct dirent *ptr ;
	int i, cnt=0 ;
	char resolved_path[64] ;
	char *path[16] ;
	char *env ;*/
	int chi_pid, status = 0 ;
	char *cmd_argv[] = { str, "test", 0 } ;

	if ((chi_pid = fork()) < 0 ) {
		printf("can't fork\n") ;
		return ERR ;
	} else if (chi_pid == 0) {
		status = execvp( str, cmd_argv ) ;
		//printf("child status: %d\n", status) ; 
		//if(status == -1)
		//	exit(status) ;
		//else
		//	exit(0) ;
		//printf("child status: %d\n", status) ;
		exit(status) ;
	} else {
		waitpid(chi_pid, &status, 0) ;
		printf("parent status: %d\n", status);
		if(status == 65280)
			return UNFIND ;
		fflush(stdout) ;
	}
	/*env = getenv("PATH");
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
	closedir(dir) ;*/
	
	return FIND ;
}

int create_one_round_pipe(int cliSock, CMD_STRUCT *cmd_list, int cmd_list_num) 
{
	int i=0, fp=-1, status;
	int fd[cmd_list_num][2] ;
	int chi_pid ;
	char buf[STR_MAX] ;

	for (i=0; i<cmd_list_num; i++) {
		if(i < (cmd_list_num-1)) {
			if (pipe(fd[i]) <0) {
				printf("can't create pipe\n") ;
				return -1 ;
			}
		} else if( i == (cmd_list_num-1) ){
			if(cmd_list[i].pipe_cross_msg == TRUE) {
				if(cli_set[cur_cli].pipe_table[cmd_list[i].pipe_cross_num][0] == -1)
					if (pipe(cli_set[cur_cli].pipe_table[cmd_list[i].pipe_cross_num]) <0) {
						printf("can't create pipe\n") ;
						return -1 ;
					}
			} else if(cmd_list[i].pipe_to_user == TRUE) {
				if(user_pipe_table[cur_cli][0] == -1)
					if (pipe(user_pipe_table[cur_cli]) <0) {
						printf("can't create pipe\n") ;
						return -1 ;
					}
			}
		}

		if ((chi_pid = fork()) < 0 ) {
			printf("can't fork\n") ;
			return -1 ;
		} else if (chi_pid == 0) {    
			if(i == 0) {
				if(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos] != NULL) {
					//printf("close pipe!\n") ;
					close(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos][1]) ;
					dup2(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos][0], 0) ;
					close(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos][0]) ;
				}
				else if(cmd_list[i].pipe_from_user != FALSE) {
					close(user_pipe_table[cmd_list[i].pipe_from_user-1][1]) ;
					dup2(user_pipe_table[cmd_list[i].pipe_from_user-1][0], 0) ;
					close(user_pipe_table[cmd_list[i].pipe_from_user-1][0]) ;
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
		
			if(i == (cmd_list_num-1)) {
				if(cmd_list[i].pipe_cross_msg == TRUE) {
					close(cli_set[cur_cli].pipe_table[cmd_list[i].pipe_cross_num][0]) ;
					dup2(cli_set[cur_cli].pipe_table[cmd_list[i].pipe_cross_num][1], 1) ;
					if(cmd_list[i].err == PIPE_OUT) {
						dup2(cli_set[cur_cli].pipe_table[cmd_list[i].pipe_cross_num][1], 2) ;
					} else
						dup2(cliSock, 2) ;
					close(cli_set[cur_cli].pipe_table[cmd_list[i].pipe_cross_num][1]) ;
				} else if(cmd_list[i].pipe_to_user == TRUE) {
					close(user_pipe_table[cur_cli][0]) ;
					dup2(user_pipe_table[cur_cli][1], 1) ;
					if(cmd_list[i].err == PIPE_OUT) {
						dup2(user_pipe_table[cur_cli][1], 2) ;
					} else
						dup2(cliSock, 2) ;
					close(user_pipe_table[cur_cli][1]) ;
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
			if(i == 0) {
				if(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos] != NULL) {
					close(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos][1]) ;
					close(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos][0]) ;
					free(cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos]) ;
					cli_set[cur_cli].pipe_table[cli_set[cur_cli].cur_pos] = NULL ;
				}
				else if(cmd_list[i].pipe_from_user != FALSE) {
					close(user_pipe_table[cmd_list[i].pipe_from_user-1][1]) ;
					close(user_pipe_table[cmd_list[i].pipe_from_user-1][0]) ;
					free(user_pipe_table[cmd_list[i].pipe_from_user-1]) ;
					user_pipe_table[cmd_list[i].pipe_from_user-1] = NULL ;
				}
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
			if(cmd_list[i].pipe_to_user == TRUE) 
			{
				memset(buf, '\0', STR_MAX) ;
				sprintf(buf, "\n*** %s (#%d) just piped '%s' into his/her pipe. ***", cli_set[cur_cli].name, cli_set[cur_cli].id, cli_set[cur_cli].rec_data) ;
				broadcast(buf) ;
			}
			if(cmd_list[i].pipe_from_user != FALSE) {
				memset(buf, '\0', strlen(buf)) ;
				sprintf(buf, "\n*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***", cli_set[cur_cli].name, cli_set[cur_cli].id, cli_set[cmd_list[i].pipe_from_user-1].name, cli_set[cmd_list[i].pipe_from_user-1].id, cli_set[cur_cli].rec_data) ;
				broadcast(buf) ;
			}
			fflush(stdout) ;
			
		}

	}
	//send_msg_to_cli(cliSock, "\0") ;
}

int decode_msg(int cliSock, char *msg)
{
	int i=0, j=0, len=0, status=0 ; 
	int last_str_type = EMPTY, pipe_extend_round=0 ; 
	int cmd_list_num=1, cmd_list_cnt=-1;
	char *ptr, *tmp ;
	char buf[STR_MAX], send_msg[STR_MAX], msg_copy[STR_MAX];
	char *input_argv[MAX_ARGV_NUM], *cmd_argv[MAX_ARGV_NUM] ;
	int input_argc=0, cmd_argc=0 ;
	CMD_STRUCT *cmd_list ;
	char *env ;
	int user_pipe_num = 0, err = 0 ;
	int id1, id2;

	//ptr = strtok( msg, " \n\r") ;
	//input_argv[input_argc] = (char *) malloc( strlen(ptr) * sizeof(char *) ) ;
	strcpy(msg_copy, msg) ;
	strcpy(cli_set[cur_cli].rec_data, strtok( msg_copy, "\n\r")) ;
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
	for(i=0; i<input_argc; i++)
	{
		if (strcmp(input_argv[i], "xch") == 0)  {
			if ( last_str_type == EMPTY ){
				if(input_argc < 3) {
					send_msg_to_cli(cliSock, "Input format failed!") ;
					return 0 ;
				}
				id1 = atoi(input_argv[i+1]) ;
				id2 = atoi(input_argv[i+2]) ;
				if((cli_survival[id1-1]==EMPTY) || (cli_survival[id2-1]==EMPTY))
				{
					send_msg_to_cli(cliSock, "No this client!\n") ;
					return 0 ;
				}

				exchange_cli_id(id1-1, id2-1) ;
				return ;
			}
		} else if (strcmp(input_argv[i], "who") == 0)  {
			if ( last_str_type == EMPTY ){
				print_all_client(cliSock) ;
				//send_msg_to_cli(cliSock, "") ;
				return 0 ;
			} else if ( last_str_type == PIPE ) { /* no this condition */
			} else if ( last_str_type == WRITE_TO_FILE) {
				//cmd_list[cmd_list_cnt].filename = input_argv[i] ;     
			} else if( last_str_type == COMMAND ) {
				cmd_argv[cmd_argc++] = input_argv[i] ;
				cmd_argv[cmd_argc] = 0 ;
			} else { /* unknown command before */
			}
		} else if (strcmp(input_argv[i], "yell") == 0) {
			if ( last_str_type == EMPTY ){
				if(input_argc < 2) {
					send_msg_to_cli(cliSock, "No input message!") ;
					return 0 ;
				}
				memset(buf, '\0', STR_MAX) ;
				sprintf(buf, "%s", input_argv[1]) ;
				for(j=2; j<input_argc; j++)
				{
					sprintf(buf, "%s %s", buf, input_argv[j]) ;
				}
				sprintf(send_msg, "\n*** %s yelled ***: %s", cli_set[cur_cli].name, buf) ;
				broadcast(send_msg) ;
				return 0;
			} else if ( last_str_type == PIPE ) { /* no this condition */
			} else if ( last_str_type == WRITE_TO_FILE) {
				//cmd_list[cmd_list_cnt].filename = input_argv[i] ;     
			} else if( last_str_type == COMMAND ) {
				cmd_argv[cmd_argc++] = input_argv[i] ;
				cmd_argv[cmd_argc] = 0 ;
			} else { /* unknown command before */
			}
		} else if (strcmp(input_argv[i], "tell") == 0) {
			if ( last_str_type == EMPTY ){
				if(input_argc < 3) {
					send_msg_to_cli(cliSock, "Input format failed!") ;
					return 0 ;
				}
				memset(buf, '\0', STR_MAX) ;
				sprintf(buf, "%s", input_argv[2]) ;
				for(j=3; j<input_argc; j++)
				{
					sprintf(buf, "%s %s", buf, input_argv[j]) ;
				}
				chat_1to1(cli_set[cur_cli].id, atoi(input_argv[1]), buf) ;
				return 0 ;
			} else if ( last_str_type == PIPE ) { /* no this condition */
			} else if ( last_str_type == WRITE_TO_FILE) {
				//cmd_list[cmd_list_cnt].filename = input_argv[i] ;     
			} else if( last_str_type == COMMAND ) {
				cmd_argv[cmd_argc++] = input_argv[i] ;
				cmd_argv[cmd_argc] = 0 ;
			} else { /* unknown command before */
			}
		} else if (strcmp(input_argv[i], "name") == 0) {
			if ( last_str_type == EMPTY ){
				if(input_argc < 2) {
					send_msg_to_cli(cliSock, "No input name!") ;
					return 0 ;
				}
				memset(buf, '\0', STR_MAX) ;
				sprintf(buf, "%s", input_argv[1]) ;
				for(j=2; j<input_argc; j++)
				{
					sprintf(buf, "%s %s", buf, input_argv[j]) ;
				}
				//free(cli_set[cur_cli].name) ;
				memset(cli_set[cur_cli].name, '\0', sizeof(cli_set[cur_cli].name)) ;
				//cli_set[cur_cli].name = (char *)malloc(strlen(buf)*sizeof(buf));
				strcpy(cli_set[cur_cli].name, buf) ;
				sprintf(buf, "\n*** User from %s/%d is named '%s'. ***", cli_set[cur_cli].ip_addr, cli_set[cur_cli].port, cli_set[cur_cli].name) ;
				broadcast(buf) ;
				return 0;
			} else if ( last_str_type == PIPE ) { /* no this condition */
			} else if ( last_str_type == WRITE_TO_FILE) {
				//cmd_list[cmd_list_cnt].filename = input_argv[i] ;     
			} else if( last_str_type == COMMAND ) {
				cmd_argv[cmd_argc++] = input_argv[i] ;
				cmd_argv[cmd_argc] = 0 ;
			} else { /* unknown command before */
			}
		} else if ((input_argv[i][0] == '|') || (input_argv[i][0] == '!')) {
			if(last_str_type == COMMAND) {
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
					pipe_extend_round = (cli_set[cur_cli].cur_pos+pipe_extend_round) % MAX_PIPE_NUM ;
					if(cli_set[cur_cli].pipe_table[pipe_extend_round] == NULL ) {
						cli_set[cur_cli].pipe_table[pipe_extend_round] = (int *)malloc(2*sizeof(int)) ;
						if(cli_set[cur_cli].pipe_table[pipe_extend_round] == NULL){
							printf("malloc() error\n");
							exit(1);
						}
						cli_set[cur_cli].pipe_table[pipe_extend_round][0] = -1 ;
						cli_set[cur_cli].pipe_table[pipe_extend_round][1] = -1 ;
					}
					cmd_list[cmd_list_cnt].pipe_cross_msg = TRUE ;
					cmd_list[cmd_list_cnt].pipe_cross_num = pipe_extend_round ;
					cmd_list[cmd_list_cnt].pipe_to_user = FALSE ;
					cmd_list[cmd_list_cnt].pipe_from_user = FALSE ;
					
				} else {
					cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
					cmd_list[cmd_list_cnt].pipe_to_user = FALSE ;
					cmd_list[cmd_list_cnt].pipe_from_user = FALSE ;
				}
			} else if(last_str_type != UNKNOWN) { 
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
				return -1 ; 
			}

		} else if((strcmp(input_argv[i], ">|")==0) || (strcmp(input_argv[i], ">!")==0)) {
			if(last_str_type == COMMAND) {
				if(i == 0) {
					memset(buf, '\0', STR_MAX) ;
					sprintf(buf, "[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
					send_msg_to_cli(cliSock, buf) ;
					printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
					return -1 ;
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

				if(cmd_list_cnt == 0)
					cmd_list[cmd_list_cnt].in = NORMAL ;
				cmd_list[cmd_list_cnt].out = PIPE_OUT ;
				cmd_list[cmd_list_cnt].err = NORMAL ;
				if(input_argv[i][1] == '!') { 
					cmd_list[cmd_list_cnt].err = PIPE_OUT ;
				}
				cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
				cmd_list[cmd_list_cnt].pipe_to_user = TRUE ;
				cmd_list[cmd_list_cnt].pipe_from_user = FALSE ;
				if(user_pipe_table[cur_cli] == NULL)
				{
					user_pipe_table[cur_cli] = (int *)malloc(2*sizeof(int)) ;
					if(user_pipe_table[cur_cli] == NULL) 
					{
						printf("malloc() error\n");
						exit(1);
					}
					user_pipe_table[cur_cli][0] = -1 ;
					user_pipe_table[cur_cli][1] = -1 ;
				} else {
					memset(buf, '\0', STR_MAX) ;
					sprintf(buf, "*** Error: your pipe already exists. ***") ;
					send_msg_to_cli(cliSock, buf) ;
					return -1 ;
				}

			} else if(last_str_type != UNKNOWN) {
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
				return -1 ;
			}
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
				if(input_argv[i][1] == '2')
					cmd_list[cmd_list_cnt].err = FILE_FD ;
				else
					cmd_list[cmd_list_cnt].err = SOCKET ;
				cmd_list[cmd_list_cnt].pipe_cross_msg = FALSE ;
				cmd_list[cmd_list_cnt].pipe_to_user = FALSE ;
				cmd_list[cmd_list_cnt].pipe_from_user = FALSE ;
				if((i+1) < input_argc) {
					//cmd_list[cmd_list_cnt].filename = input_argv[i+1] ;
					strcpy(cmd_list[cmd_list_cnt].filename,  input_argv[i+1]) ;
					break ;
				}
			} else if(last_str_type != UNKNOWN) {
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
				return -1 ;
			}
		} else if (input_argv[i][0] == '<') {	
			if(last_str_type == COMMAND) {
				ptr = &input_argv[i][1] ;
				user_pipe_num = atoi(ptr);
				if((user_pipe_num <= 0) || (user_pipe_num > MAX_CLIENT_NUM) 
					|| (user_pipe_table[user_pipe_num-1] == NULL))
				{
					memset(buf, '\0', strlen(buf)) ;
					sprintf(buf, "*** Error: the pipe from #%d does not exist yet. ***", user_pipe_num) ;
					send_msg_to_cli(cliSock, buf) ;
					return -1 ;
				}
				//memset(buf, '\0', strlen(buf)) ;
				//sprintf(buf, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***", cli_set[cur_cli].name, cli_set[cur_cli].id, cli_set[user_pipe_num-1].name, cli_set[user_pipe_num-1].id, msg_copy) ;
				//broadcast(buf) ;
			} else if(last_str_type != UNKNOWN){
				printf("[ERR]: syntax error near unexpected token '%s'", input_argv[i]) ;
				return -1 ;
			}
		} else if (strcmp(input_argv[i], "setenv") == 0) {
			if( last_str_type == EMPTY ){
				if((i+2) < input_argc) {
					/*if(setenv(input_argv[i+1], input_argv[i+2], 1) == -1) {
					  printf("Set failed\n") ;
					  send_msg_to_cli(cliSock, "Setenv Failed!\n") ;
					  }*/
					if(set_env(input_argv[i+1], input_argv[i+2]) == -1 ) {
						fprintf(stderr, "[ERR] TOO MUCH ENVP!") ;
						//printf("Set failed\n") ;
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
                                        if((env = get_env(input_argv[i+1])) == NULL )				
					{
						fprintf(stderr, "[ERR] CAN'T FIND THIS ENVP!") ;
						send_msg_to_cli(cliSock, "Can't find this envp!\n") ;
						return 0 ;
					}
                                        //len = strlen(env) ;
                                        //env[len] = '\n' ;
                                        //env[len+1] = '\0' ;
                                        //printf("env aft: %s\n", env) ;
                                        memset(buf, '\0', STR_MAX) ;
        				sprintf(buf, "%s=%s\n", input_argv[i+1], env);
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
					cmd_argv[0] = input_argv[i] ;
					cmd_argv[1] = 0 ;
					cmd_argc = 1 ;
					last_str_type = COMMAND ;
				}
			} else { /* unknown command */
				if((last_str_type==PIPE) && (err==0))
				{
					cmd_list[cmd_list_cnt].out = SOCKET ;
					cmd_list[cmd_list_cnt].err = SOCKET ;
					cmd_list_num = cmd_list_cnt+1 ;
					break ;
				}
				if((err == 1) && (cmd_list_cnt != -1))
					break ;
				memset(buf, '\0', strlen(buf)) ;
                        	sprintf(buf, "Unknown command: [%s].\n", input_argv[i]);
                        	send_msg_to_cli(cliSock, buf) ;
				//return 0 ;
				err = 1;
				last_str_type = UNKNOWN ;
			}
		} 
	}

	if((cmd_list_cnt == -1) && (last_str_type == UNKNOWN))
	{
		printf("[ERR] NO COMMAND TO EXEC!\n") ;
		return ;
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
		cmd_list[cmd_list_cnt].pipe_to_user = FALSE ;
		if(user_pipe_num == 0)
			cmd_list[cmd_list_cnt].pipe_from_user = FALSE ;
		else 
			cmd_list[cmd_list_cnt].pipe_from_user = user_pipe_num ;
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

	//memset(buf, '\0', STR_MAX) ;
	//sprintf(buf, "%s%s", msg, symbol);
	if((nByteWrite = send(cliSock, msg, strlen(msg), 0)) == -1)
	{
		printf("[ERR] Send to client fault.\n");
		return -1;
	}
	printf("nByteWrite: %d, Send message to client: %s\n", nByteWrite, buf);
	//memset(buf, '\0', strlen(buf)) ;
	fflush(stdout);	
	
	return 0;
}

int find_min_empty_cli() 
{
	int i=0;
	for(i=0; i<MAX_CLIENT_NUM; i++)
	{
		if(cli_survival[i] == EMPTY)
			return i;
	}
	return -1;
}

int find_cli_fd_num(int sock)
{
	int i = 0 ;
	for(i=0; i<MAX_CLIENT_NUM; i++) 
	{
		if(cli_set[i].sock == sock)
			return i;
	}
	return -1;
}
int set_env(char *env_name, char *env_content) 
{
	int i = 0 ;
	for(i=0; i<cli_set[cur_cli].cur_env_num; i++)
	{
		if(strcmp(cli_set[cur_cli].env[i].name, env_name) == 0)
		{
			memset(cli_set[cur_cli].env[i].path, '\0', sizeof(cli_set[cur_cli].env[i].path)) ;
			//free(cli_set[cur_cli].env[i].path) ;
			//cli_set[cur_cli].env[i].path = (char *)malloc(strlen(env_content) * sizeof(char *)) ;
			strcpy(cli_set[cur_cli].env[i].path, env_content) ;

			//cli_set[cur_cli].env[i].path = env_content ;
			return NO_ERR ;
		}
		/*else if(cli_set[cur_cli].env[i].name == NULL)
		{
			cli_set[cur_cli].env[i].name = env_name ;
			cli_set[cur_cli].env[i].path = env_content ;
			return NO_ERR;
		}*/
	}
	if(i == MAX_ENV_NUM)
		return ERR ;
	//cli_set[cur_cli].env[i].name = (char *)malloc(strlen(env_name) * sizeof(char *)) ;
	memset(cli_set[cur_cli].env[i].name, '\0', sizeof(cli_set[cur_cli].env[i].name)) ;
	strcpy(cli_set[cur_cli].env[i].name, env_name) ;
	//cli_set[cur_cli].env[i].path = (char *)malloc(strlen(env_content) * sizeof(char *)) ;
	memset(cli_set[cur_cli].env[i].path, '\0', sizeof(cli_set[cur_cli].env[i].path)) ;
	strcpy(cli_set[cur_cli].env[i].path, env_content) ;
	cli_set[cur_cli].cur_env_num++ ;
	//printf("which env: %d, name: %s, path: %s\n", i, cli_set[cur_cli].env[i].name, cli_set[cur_cli].env[i].path) ;
	return NO_ERR ;
	//fprintf(stderr, "[ERR] TOO MUCH ENVP!") ;
	//return ERR ;
}

char *get_env(char *env_name) 
{
	int i = 0 ;
	//printf("cur_env_num: %d\n", cli_set[cur_cli].cur_env_num) ;
	for(i=0; i<cli_set[cur_cli].cur_env_num; i++)
	{
		//printf("name1: %s, name2: %s\n", cli_set[cur_cli].env[i].name, env_name) ;
		if(strcmp(cli_set[cur_cli].env[i].name, env_name) == 0)
		{
			//printf("path name match\n");
			return cli_set[cur_cli].env[i].path ;
		}
	}
	return NULL ;
}

void print_all_client(int cliSock)
{
	int i = 0 ;
	char buf[STR_MAX] ;
	sprintf(buf, "\n<ID>\t<nickname>\t<IP/port>\t<indicate me>");
	if(send(cliSock, buf, strlen(buf), 0) == -1)
	{
		printf("[ERR] Send to client fault.\n");
		//return;
	}
	for(i=0; i<MAX_CLIENT_NUM; i++)
	{
		if(cli_set[i].sock == cliSock)
		{
			memset(buf, '\0', STR_MAX) ;
			sprintf(buf, "\n%d\t%s\t%s/%d <- me", cli_set[i].id, cli_set[i].name, cli_set[i].ip_addr, cli_set[i].port);
			if(send(cliSock, buf, strlen(buf), 0) == -1)
			{
				printf("[ERR] Send to client fault.\n");
				//return -1;
			}
		}
		else
			if(cli_survival[i] == EXIST)
			{
				memset(buf, '\0', STR_MAX) ;
				sprintf(buf, "\n%d\t%s\t%s/%d", cli_set[i].id, cli_set[i].name, cli_set[i].ip_addr, cli_set[i].port);
				if(send(cliSock, buf, strlen(buf), 0) == -1)
				{
					printf("[ERR] Send to client fault.\n");
					//return -1;
				}
				//send_msg_to_cli(cli_set[i].sock,  msg) ;
			}
	}
	//send_msg_to_cli(cliSock, "") ;
}

char name[32] ;
char ip_addr[32] ;
char rec_data[STR_MAX] ;
int port ;
int sock ;
int id ;
int cur_pos ;
int *pipe_table[MAX_PIPE_NUM] ;
int cur_env_num ;
ENV_STRUCT env[MAX_ENV_NUM] ;

void exchange_cli_id(int id1, int id2)
{
	int i = 0;
	CLIENT_STRUCT tmp ;
	strcpy(tmp.name, cli_set[id1].name) ;
	strcpy(tmp.ip_addr, cli_set[id1].ip_addr) ;
	tmp.port = cli_set[id1].port ;
	tmp.sock = cli_set[id1].sock ;
	tmp.cur_pos = cli_set[id1].cur_pos ;
	for(i=0; i<MAX_PIPE_NUM; i++)
	{
		if(cli_set[id1].pipe_table[i] != NULL)
			tmp.pipe_table[i] = cli_set[id1].pipe_table[i] ;
	}
	tmp.cur_env_num = cli_set[id1].cur_env_num ;
	for(i=0; i<tmp.cur_env_num; i++)
	{
		strcpy(tmp.env[i].name, cli_set[id1].env[i].name) ;
		strcpy(tmp.env[i].path, cli_set[id1].env[i].path) ;
	}

	strcpy(cli_set[id1].name, cli_set[id2].name) ;
	strcpy(cli_set[id1].ip_addr, cli_set[id2].ip_addr) ;
	cli_set[id1].port = cli_set[id2].port ;
	cli_set[id1].sock = cli_set[id2].sock ;
	cli_set[id1].cur_pos = cli_set[id2].cur_pos ;
	for(i=0; i<MAX_PIPE_NUM; i++)
	{
		if(cli_set[id2].pipe_table[i] != NULL)
			cli_set[id1].pipe_table[i] = cli_set[id2].pipe_table[i] ;
	}
	cli_set[id1].cur_env_num = cli_set[id2].cur_env_num ;
	for(i=0; i<cli_set[id1].cur_env_num; i++)
	{
		strcpy(cli_set[id1].env[i].name, cli_set[id2].env[i].name) ;
		strcpy(cli_set[id1].env[i].path, cli_set[id2].env[i].path) ;
	}

	strcpy(cli_set[id2].name, tmp.name) ;
	strcpy(cli_set[id2].ip_addr, tmp.ip_addr) ;
	cli_set[id2].port = tmp.port ;
	cli_set[id2].sock = tmp.sock ;
	cli_set[id2].cur_pos = tmp.cur_pos ;
	for(i=0; i<MAX_PIPE_NUM; i++)
	{
		if(tmp.pipe_table[i] != NULL)
			cli_set[id2].pipe_table[i] = tmp.pipe_table[i] ;	
	}
	cli_set[id2].cur_env_num = tmp.cur_env_num ;
	for(i=0; i<cli_set[id2].cur_env_num; i++)
	{
		strcpy(cli_set[id2].env[i].path, tmp.env[i].path) ;
		strcpy(cli_set[id2].env[i].name, tmp.env[i].name) ;
	}

}

void chat_1to1(int from, int to, char *msg)
{
	char buf[STR_MAX] ;
	if((to <= 0) || (to > MAX_CLIENT_NUM) || (cli_survival[to-1] == EMPTY)) 
	{
		//memset(buf, '\0', STR_MAX) ;
		sprintf(buf, "\n*** Error: user #%d does not exist yet. ***\n", to) ;
		send_msg_to_cli(cli_set[from-1].sock, buf) ;
	}
	else
	{
		sprintf(buf, "\n*** %s told you ***: %s\n", cli_set[from-1].name, msg) ;
		send_msg_to_cli(cli_set[to-1].sock, buf) ;
	}
}

void broadcast(char *msg) 
{
	int i = 0 ;
	for(i=0; i<MAX_CLIENT_NUM; i++)
	{
		if(cli_survival[i] == EXIST)
		{
			send_msg_to_cli(cli_set[i].sock, msg) ;				
		}
	}
}


