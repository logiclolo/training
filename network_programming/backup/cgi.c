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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/stat.h>


#define MAXUSER 5
#define MAX_MESSAGE 3000
#define MAX_ARGV_NUM 100
#define EXIT_STR "exit\r\n"
#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3

char *input_argv1_global[MAX_ARGV_NUM];
int input_argc_global = 0;
int status[MAXUSER];

int recv_msg(int userno,int from)
{
    char buf[3000],*tmp,*a;
    int len,i;
    if((len=read(from,buf,sizeof(buf)-1)) <0) 
        return -1;
        
    buf[len] = 0;
    if(len>0)
    {
        for(tmp=strtok(buf,"\n");tmp;tmp=strtok(NULL,"\n"))
        {
            //printf("\n---------------user %d---------------\n",userno); 
            //printf("%d| %s\n",userno,tmp);	//echo input
			

			//if(tmp[0] == '\r'){
			//	printf("<script>document.all['m%d'].innerHTML +=  <br>\";</script>\n",userno);	
			//	fflush(stdout);
			//}
			//else{
				a = tmp;
				while(*a!='\0'){
					//printf("char is %d\n",*a);
					if(a[0] == '\r')
						a[0] = 0;
						a++;
				}
				printf("<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n",userno,tmp);
				fflush(stdout);
			//}
			
			
			if(tmp[0] == '%')
				status[userno] = F_WRITING;		
        }
    }
    fflush(stdout); 
    return len;
}

int readline(int fd,char *ptr,int maxlen)
{
        int n,rc;
	char c;
        *ptr = 0;
        for(n=1;n<maxlen;n++)
        {
        	if((rc=read(fd,&c,1)) == 1)
        	{
        		*ptr++ = c;
        		if(c=='\n')  break;
        	}
        	else if(rc==0)
        	{
        		if(n==1)     return(0);
        		else         break;
        	}
        	else
        		return(-1);
        }
        return(n);
}      
  

void print_to_browser(char* buf,int num){
	
	//char percent[]="%";	
	//printf("---%s",buf);	
	input_argv1_global[input_argc_global] = strtok(buf,"\n");
	input_argv1_global[input_argc_global][strlen(input_argv1_global[input_argc_global])] = '\0';
	//printf("%s\n",input_argv1_global[input_argc_global]);
	printf(":<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n",num,input_argv1_global[input_argc_global]);
	fflush(stdout);

	while(input_argv1_global[++input_argc_global] = strtok(NULL,"\n")){
	   input_argv1_global[input_argc_global][strlen(input_argv1_global[input_argc_global])] = '\0' ;
	   //printf("%s\n",input_argv1_global[input_argc_global]);
	   if(input_argv1_global[input_argc_global][0]=='%'){
	   		//printf("<script>document.all['m%d'].innerHTML += \"%s \";</script>\n",num,percent);
			printf("fuck\n");
			fflush(stdout);
			status[num] = F_WRITING;
			break;
		}
	   printf("<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n",num,input_argv1_global[input_argc_global]);
	   fflush(stdout);
	
	}
		printf("\n");
		fflush(stdout);

}

int main(int argc,char *argv[])
{
    fd_set readfds;
    fd_set rs;
	fd_set writefds;
	fd_set ws;
    int    client_fd[MAXUSER];
    struct sockaddr_in client_sin[MAXUSER];
    char buf[3000];
    char msg_buf[MAX_MESSAGE],msg_buf1[3000];
	char host1[30] = "ep.cs.nthu.edu.tw";
	char host2[30] = "pegasus.cs.nthu.edu.tw";
	char batch_file1[10] = "test1.txt";
    int len;
    int SERVER_PORT;
    FILE *fd; 
    int i,end;
    struct hostent *he;
	int conn=0;
 	int nfds = FD_SETSIZE;
	//int status[MAXUSER];
	int AlreadyWrite[MAXUSER];
	int NeedWrite[MAXUSER];
	int file_fd[MAXUSER];
	int flag,n,error;
	struct stat file_stat;
	struct timeval timeout;
	char env[256];
	char *input_argv[MAX_ARGV_NUM];
	char *input[MAX_ARGV_NUM];
	//int input = 0;
	int input_argc = 0;
	int num_server = -1;
	char *batch_file[MAXUSER];
	int  port[MAXUSER];
	char *hostname[MAXUSER];
	int file;
	char *check;
	char cgi_text[] =
	"\
	<html>\n\
	<head>\n\
	<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n\
	<title>Network Programming Homework 3</title>\n\
	</head>\n\
	<body bgcolor=#336699>\n\
	<font face=\"Courier New\" size=2 color=#FFFF99>\n\
	<table width=\"800\" border=\"1\">\n\
	<tr>\n\
	<td valign=\"top\" id=\"a0\"></td><td valign=\"top\" id=\"a1\"></td><td valign=\"top\" id=\"a2\"></td> <td valign=\"top\" id=\"a3\"></td><td valign=\"top\" id=\"a4\"></td></tr>\n\
	<tr>\n\
	<td valign=\"top\" id=\"m0\"></td><td valign=\"top\" id=\"m1\"></td><td valign=\"top\" id=\"m2\"></td><td valign=\"top\" id=\"m3\"></td><td valign=\"top\" id=\"m4\"></td></tr>\n\
	</table>\n\
	</font>\n\
	</body>\n\
	</html>\n\
	" ;

	char percent[]="%";
	
	
	printf("Content-type:text/html\n\n");
	fflush(stdout);
	
	



	
	//if((file=open("index.htm",O_RDONLY))==-1)
	//             write(1, "Failed to open file", 19);
	//while ((i=read(file, buf, MAX_MESSAGE))>0) {
	//        write(1,buf,i);
	//}

	
	//fflush(stdout);
	printf("%s",cgi_text);
	fflush(stdout);

    /*
	if(argc == 3)
        fd = stdin;
    else if(argc == 4)
        fd = fopen(argv[3], "r");
    else
    {
        fprintf(stderr,"Usage : client <server ip> <port> <testfile>\n");
        exit(1);
    }
	*/
	/*
    if((he=gethostbyname(argv[1])) == NULL)
    {
        fprintf(stderr,"Usage : client <server ip> <port> <testfile>");
        exit(1);
    }*/
                              
    //SERVER_PORT = (u_short)atoi(argv[2]);
 
   
	

	timeout.tv_sec  = 0;
	timeout.tv_usec = 1;

	FD_ZERO(&readfds);
	FD_ZERO(&rs);
	FD_ZERO(&writefds);
	FD_ZERO(&ws);

	//strcpy(env,getenv("QUERY_STRING"));
	//strcpy(env,"h1=140.114.78.62=&p1=8002&f1=test1.txt&h2=ep.cs.nthu.edu.tw&p2=8001&f2=test1.txt&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5= HTTP/1.1");
	strcpy(env,"h1=140.114.78.62&p1=7000&f1=t3.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5= HTTP/1.1");
	//printf("%s\n",env);
	//fflush(stdout);
	
	// parse the QUERY_STRING
	input_argv[input_argc] = strtok(env,"&");
	input_argv[input_argc][strlen(input_argv[input_argc])] = '\0';

	while(input_argv[++input_argc] = strtok(NULL,"&"))
		input_argv[input_argc][strlen(input_argv[input_argc])] = '\0' ;
	
	
	
	//printf("input_argc is %d\n",input_argc);
	fflush(stdout);

	for(i=0;i<input_argc;i++){
		//printf("input_argv[%d] is %s\n",i,input_argv[i]);
		if(input_argv[i][0] == 'h' && (input_argv[i][3] != '\0' && input_argv[i][3] != ' ')){
			num_server++;
			hostname[num_server] = input_argv[i]+3;
			printf("<script>document.all['a%d'].innerHTML += \" <b>%s</b><br>\";</script>\n",num_server,hostname[num_server]);
			//printf("  hostname[%d] is %s\n",num_server,hostname[num_server]);
		}
		if(input_argv[i][0] == 'p' && (input_argv[i][3] != '\0' && input_argv[i][3] != ' ')){
			port[num_server] = atoi(input_argv[i]+3);
			//printf("  port[%d] is %d\n",num_server,port[num_server]);
		}
		if(input_argv[i][0] == 'f' && (input_argv[i][3] != '\0' && input_argv[i][3] != ' ')){
			batch_file[num_server]=input_argv[i]+3;
			//printf("  batch_file[%d] is %s\n",num_server,batch_file[num_server]);

		}


	}
	//printf("num_server is %d\n",num_server);

	conn = num_server+1;
	
		
	for(i=0;i<conn;i++)
    {
        
		he = gethostbyname(hostname[i]);

		fd = fopen(batch_file[i],"r");
		file_fd[i] = fileno(fd);
		stat(batch_file[i],&file_stat);
		NeedWrite[i] = file_stat.st_size;
		//printf("%s's size is %d\n",batch_file1,NeedWrite[i]);

		client_fd[i] = socket(AF_INET,SOCK_STREAM,0);
        bzero(&client_sin[i],sizeof(client_sin[i]));
        client_sin[i].sin_family = AF_INET;
        client_sin[i].sin_addr = *((struct in_addr *)he->h_addr); 
        client_sin[i].sin_port = htons(port[i]);
		//flag = fcntl(client_fd[i], F_GETFL, 0);
		//printf("flag is %d\n",flag);
		//if(fcntl(client_fd[i], F_SETFL, flag | O_NONBLOCK) < 0)
		//	printf("non blocking error!!\n");
		fcntl(client_fd[i],F_SETFL, O_NONBLOCK);
		//printf("sockfd[%d] is %d\n",i,client_fd[i]);
		if(connect(client_fd[i],(struct sockaddr *)&client_sin[i],sizeof(client_sin[i])) < 0){
			//printf("connection wrong\n");
			if(errno != EINPROGRESS){
				printf("connection failed\n");
				fflush(stdout);
				return -1;
			
			}
    	}
		//usleep(300000);
		FD_SET(client_fd[i],&rs);
		FD_SET(client_fd[i],&ws);
		//FD_SET(file_fd[i],&rs);
			
		status[i]=F_CONNECTING;	
		
	}


    
   
    

    
	readfds = rs;
	writefds = ws;


	
	//FD_SET(fileno(fd),&rs);//設檔案
    
	
	
	//printf("connection number is %d\n",conn);	
    while(conn)
    { 
        memcpy(&readfds,&rs,sizeof(readfds));
        memcpy(&writefds,&ws,sizeof(writefds));
		        			
		
		
		
		
		
		if(select(nfds,&readfds,&writefds,NULL,&timeout) < 0)
           exit(1);
		//usleep(1200000); sleep 4 secs
		//printf("hahahaha\n");
		
		
		for(i=0;i<(num_server+1);i++){
			if(FD_ISSET(client_fd[i],&readfds)){
			    memset(msg_buf,'\0',MAX_MESSAGE);
		        fcntl(client_fd[i],F_SETFL, O_NONBLOCK);
		        //n = recv(client_fd[i],msg_buf,MAX_MESSAGE,0);
				n = recv_msg(i,client_fd[i]);
				
				//usleep(200000);

		         if(n<=0){
				 //read finished
		         printf("no.%d read finished!\n",i);
		         FD_CLR(client_fd[i],&rs);
		         status[i] = F_DONE;

		         conn--;
		         }
		    }
		}
		
		
		
		
		
		
		
		
		for(i=0;i<(num_server+1);i++){
			
			if(status[i] == F_CONNECTING && (FD_ISSET(client_fd[i],&readfds) || FD_ISSET(client_fd[i],&writefds))){
				if (getsockopt(client_fd[i], SOL_SOCKET, SO_ERROR, &error, &n) < 0 ||error != 0) {
				// non-blocking connect failed
				return (-1);
				}
				status[i] = F_WRITING;
				FD_CLR(client_fd[i],&rs);
			
			
			}
			else if(status[i] == F_WRITING && FD_ISSET(client_fd[i],&writefds)){

				//read from file
				len = readline(file_fd[i],msg_buf,sizeof(msg_buf));
				if(len < 0)
					exit(1);
			


				msg_buf[len-1] = 13;
				msg_buf[len] = 10;
				msg_buf[len+1] = '\0';
				//fflush(stdout);
				//printf("(no.%d) command:%s",i,msg_buf);
				//write(1,buf,strlen(msg_buf1)+1);
				//printf("%s",msg_buf1);
				fflush(stdout);
				AlreadyWrite[i] = write(client_fd[i],msg_buf,strlen(msg_buf));
				NeedWrite[i] -= AlreadyWrite[i];
				FD_SET(client_fd[i],&rs);
				status[i] = F_READING;	
				
				
				
				//eliminate '\r'
				check = msg_buf;
				while(*check != '\0'){
					if(check[0] == '\r')
						check[0] = '\0';
					check++;
				
				}


				printf("<script>document.all['m%d'].innerHTML += \" <b>%s</b><br>\";</script>\n",i,msg_buf);
				fflush(stdout);

				//usleep(100000);

				//if(AlreadyWrite[i]<=0 || NeedWrite[i]<=0){
				if(!strncmp(msg_buf,"exit",4)){
					//write finished
					printf("no.%d write finished!\n",i);
					FD_CLR(client_fd[i],&ws);
					status[i] = F_READING;
					FD_SET(client_fd[i],&rs);
				}
				
			
			}
			//else if(status[i] == F_READING && FD_ISSET(client_fd[i],&readfds)){
			//else if(status[i] == F_READING){
		/*	if (FD_ISSET(client_fd[i],&readfds)){
				memset(msg_buf,'\0',MAX_MESSAGE);
				usleep(300000);
				//printf("before reading\n");
				//fflush(stdout);
				//n = recv(client_fd[i],msg_buf,sizeof(msg_buf)-1,0);
				fcntl(client_fd[i],F_SETFL, O_NONBLOCK);
				n = recv(client_fd[i],msg_buf,MAX_MESSAGE,0);
				printf("no.%d @@@@@@read is %d\n",i,n);
				printf("%s",msg_buf);
				fflush(stdout);
				if(n<=0){
					//read finished
					printf("no.%d read finished!\n",i);
					FD_CLR(client_fd[i],&rs);
					status[i] = F_DONE;
					conn--;
				}

			}*/	
			
		}//end of for
		

		/*
        for(i=0;i<MAXUSER;i++)
        {  
            if(FD_ISSET(client_fd[i],&readfds))
            {//接收server送來的訊息
                //接收message
                if(recv_msg(i,client_fd[i]) <0)
                {
                    shutdown(client_fd[i],2);
                    close(client_fd[i]);
                    exit(1);
                }
            }
        }
  
        if(FD_ISSET(fileno(fd),&readfds))
        {//從檔案
            //送meesage
            len = readline(fileno(fd),msg_buf,sizeof(msg_buf));
            if(len < 0) 
                exit(1);
            
            msg_buf[len-1] = 13;
            msg_buf[len] = 10;
            
            msg_buf[len+1] = '\0';
            fflush(stdout);
            if(!strncmp(msg_buf,"exit",4))
            {//全部離開
                printf("\n%s",msg_buf);
                sleep(2);	//waiting for server messages
                for(i=0;i<MAXUSER;i++)
                {
                    if (FD_ISSET(client_fd[i],&rs))//modify by sapp
                    {
                        if(write(client_fd[i],EXIT_STR ,6) == -1) return -1;
                        while(recv_msg(i,client_fd[i]) >0);
                        shutdown(client_fd[i],2);
                        close(client_fd[i]);
                        FD_CLR(client_fd[i], &rs);
                    }
                }
                FD_CLR(fileno(fd), &rs);
                fclose(fd);
                exit(0);
                //end =1;//del by sapp
            }
            else if(!strncmp(msg_buf,"login",5))
            {//登入
                printf("\n%s",msg_buf);
                //i=msg_buf[5]-'0'; //del by fou                
                sscanf(msg_buf, "login%d", &i); // modify by fou
                if(i<MAXUSER&&i>=0)
                {
                    if(connect(client_fd[i],(struct sockaddr *)&client_sin[i],sizeof(client_sin[i])) == -1)
                    {
                        perror("");
                        printf("connect fail\n");
                    }
                    else
                        FD_SET(client_fd[i],&rs);
                }
            }
            else if (!strncmp(msg_buf,"logout",6)) //add by fou
            {//登出
                printf("\n%s",msg_buf);
                sscanf(msg_buf, "logout%d", &i);
                if(i<MAXUSER&&i>=0)
                {
                    if (FD_ISSET(client_fd[i],&rs))//modify by sapp
                    {
                        if(write(client_fd[i], EXIT_STR,6) == -1) return -1;
                        while(recv_msg(i,client_fd[i]) >0 );
                        shutdown(client_fd[i],2);
                        close(client_fd[i]);
                        FD_CLR(client_fd[i], &rs);
                        FD_CLR(client_fd[i],&readfds);
                        client_fd[i] = socket(AF_INET,SOCK_STREAM,0);
                    }
                }
            }  
            else
            {//送cmd
                char tmpArr[20];
                // i=msg_buf[0]-'0'; //del by fou
                sscanf(msg_buf, "%d", &i);//modify by fou                
                sprintf(tmpArr, "%d", i);         
               	strcpy(msg_buf1,&msg_buf[strlen(tmpArr)+1]);
                //	strcpy(msg_buf1,&msg_buf[2]); 
                printf("\n%d %% %s",i,msg_buf1);
                if(i<MAXUSER){
                		//write(client_fd[i],msg_buf1,len-1,0);
                    write(client_fd[i],msg_buf1,strlen(msg_buf1));
                }
            }   
            usleep(300000);       //sleep 1 sec before next commandlready at oldest change 
			
        }*/

    } // end of while

	printf("END\n");
	fflush(stdout);
	return 0 ;
}  // end of main

