#include<stdlib.h>
#include<stdio.h>

int main()
{
  int pid = fork();
  if(pid) 
    printf("%d, I'm parent\n",pid);
  else if(pid == 0){
    int tmp = 0;
    while(tmp==0);
    printf("%d, I'm child\n",pid);
 
  }
  else
    printf("fork error\n");


  return 0;

}
