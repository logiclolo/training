#include<stdlib.h>
#include<stdio.h>
#include<arpa/inet.h>

#define MAXLINE 512

int sockfd,newsockfd,clilen,childpid;


int main(int argc,char *argv[])
{

  struct sockaddr_in cli_addr,serv_addr;
  int pipe1[2];

  if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
    err_dump("server:can't open stream socket");

  bzero((char*)&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(7000);

  if(bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    err_dump("server:can't bind local address");

  listen(sockfd,5);



  //if(pipe(piep1) < 0)
 //   err_sys("can't create pipes");
    
  while(1){
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
    if(newsockfd < 0){
      err_dump("server:accept error");
      exit(1);
    }  
    if((childpid = fork()) < 0)
      err_dump("server:fork error");
    else if(childpid == 0){
      close(sockfd);
      str_echo(newsockfd);
      exit(0);
    }
    printf("father!!!");
    close(newsockfd);
  }

}


void str_echo(int sockfd)
{
  int count=1;
  int n;
  char line[MAXLINE];
  char str[]="hellooooo!!!\n";
  write(sockfd,str,sizeof(str));
  //n=read(sockfd,line,MAXLINE);

   

   n=read(sockfd,line,MAXLINE);
   printf("%s\n",line);
  
    //{
     // printf("%s\n%d\n",line,n);
     // fflush(stdout);
    //}
  //  if(n == 0)
 //     return;
 //   else if(n < 0)
 //     err_dump("str_echo:readline error");
 //   if(write(sockfd,line,n)!= n)
 //     err_dump("str_echo:writen error");
   
    
 // }


}







err_dump(char *string)
{
  printf("%s\n",string);
  perror("system error message:");
  close(sockfd);
  close(newsockfd);
}

/*
int readline(int fd,char *ptr,int maxlen)
{
  int n,nc;
  char c;
  *ptr = 0;

  for(n=1;n<maxlen;n++){
    if((rc=read(fd,&c,1)==1)){
      *ptr++ = c;

    }











}



*/

