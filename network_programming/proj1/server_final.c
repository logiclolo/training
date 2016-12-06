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

#define NORMAL		0
#define	PIPE_IN		1
#define	PIPE_OUT	2
#define FILE_FD		3
#define	SOCKET		4
#define PASS_ERR	5

struct cmd_struct {
	char **argv ;
	int in ;
	int out ;
	int err ;
	int pipe_cross_msg ;
	int pipe_cross_num ;
	char filename[64] ;
} ;

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
CMD_STRUCT *cmd_list ;


char welcome[] = 
"\
****************************************\n\
** Welcome to the information server. **\n\
****************************************\n\
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
    	char buf[1024] ;
	char symbol[] = "% ", exit_msg[] = "exit \n" ;
	char *env ;
	int i=0 ; 
	char *temp ;
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
	while(1) {
		/* Waiting for a incoming connection */
		cliSock=accept(srvSock, (struct sockaddr*)&cliSockaddr, &nsockaddrsize) ;
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
			
			// set path
			setenv("PATH", "bin:.", 1) ;
			realpath("./ras", resolved_path) ;
			chdir(resolved_path) ;

			printf("[011] Waiting for client message") ;
			printf("*******\n");

			/* if you want to see the ip address and port of the client, uncomment the 
			   next two lines */

			//	printf("Hi there, from  %s#\n",inet_ntoa(srvSockaddr.sin_addr));
			printf("Coming from port %d\n\n",ntohs(srvSockaddr.sin_port));
			sprintf(buf, "%s\n%s", welcome, symbol);
			//printf("Send message to client: %s\n", buf);
			nRetValue = send(cliSock, buf, strlen(buf), 0);
			memset(buf, '\0', strlen(buf)) ;

			while(1) {
				FD_ZERO(&readfds);
				FD_SET(cliSock,&readfds);

				if (select(cliSock+1, &readfds, NULL, NULL, NULL) < 0) {
					printf("[ERR] select failed!\n") ;
					exit(1) ;
				}
				/* get a message from the client */
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
						break ;


				}
				fflush(stdout) ;
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
		}

		if ((chi_pid = fork()) < 0 ) {
			printf("can't fork\n") ;
			return -1 ;
		} else if (chi_pid == 0) {    
			if(i == 0)
				if(pipe_table[cur_pos] != NULL) {
					close(pipe_table[cur_pos][1]) ;
					dup2(pipe_table[cur_pos][0], 0) ;
					close(pipe_table[cur_pos][0]) ;
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
	printf("input_argc is %d\n",input_argc);
	for(i=0; i<input_argc; i++)
	{
		if ((input_argv[i][0] == '|') || (input_argv[i][0] == '!')) {
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

