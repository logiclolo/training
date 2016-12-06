#include <windows.h>
//#include <stats.h>
#include <list>
using namespace std;

#include "resource.h"
#include <time.h>

//#include "string.h"



#define SERVER_PORT 7799
#define BUFSIZE 8096
#define MAX_ARGV_NUM 100
#define MAXUSER 6
//#define EXIT_STR "exit\r\n"
#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3

#define WM_SOCKET_NOTIFY (WM_USER + 1)
#define WM_SOCKET_NOTIFY_C0 (WM_USER + 2)
#define WM_SOCKET_NOTIFY_C1 (WM_USER + 3)
#define WM_SOCKET_NOTIFY_C2 (WM_USER + 4)
#define WM_SOCKET_NOTIFY_C3 (WM_USER + 5)
#define WM_SOCKET_NOTIFY_C4 (WM_USER + 6)

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
void handle_socket(int fd,static HWND hwnd);
int EditPrintf (HWND, TCHAR *, ...);
char *mySavestring (char *ptr, size_t size);
void parse_env(char *msg);
BOOL SendFile(int s, char* fname);
int readline(FILE* fd,char *ptr,int maxlen);
int recv_msg(int userno,int from,static HWND Edithwnd,static HWND hwnd);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;
char buffer[BUFSIZE+1];
char commandbuffer[500];
char bufferParse[BUFSIZE+1];
char query_string[1000];

int j, file_fd, buflen, len;
unsigned long val;
long i, ret;
char * fstr;
int status;
FILE *file;
struct hostent *he;
int    client_fd[MAXUSER];
struct sockaddr_in client_sin[MAXUSER];
char *input_argv[MAX_ARGV_NUM];
char *input[MAX_ARGV_NUM];
int conn=0;
int input_argc = 0;
int num_server = -1;
char *batch_file[MAXUSER];
int  port[MAXUSER];
char *hostname[MAXUSER];
FILE *filefd[MAXUSER];
int Mystatus[MAXUSER];
char *check;
static SOCKET msock, ssock;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static HWND hwndEdit;
	
	static struct sockaddr_in sa;
	

	int err;


	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
					break;
				case FD_READ:
				//Write your code for read event here.
					int bytes;	
					
					//bytes = recv(ssock,buf,sizeof(buf),0);
					//buf[bytes] = '\0';
					handle_socket(ssock,hwndEdit);
					EditPrintf(hwndEdit, TEXT("=== reading ===\r\n"));		
		

					err = WSAAsyncSelect(client_fd[0], hwnd, WM_SOCKET_NOTIFY_C0, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					err = WSAAsyncSelect(client_fd[1], hwnd, WM_SOCKET_NOTIFY_C1, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					err = WSAAsyncSelect(client_fd[2], hwnd, WM_SOCKET_NOTIFY_C2, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					err = WSAAsyncSelect(client_fd[3], hwnd, WM_SOCKET_NOTIFY_C3, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					err = WSAAsyncSelect(client_fd[4], hwnd, WM_SOCKET_NOTIFY_C4, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
		
					//if ( err == SOCKET_ERROR ) {
					//	EditPrintf(hwnd, TEXT("=== Error: select error!!!!!! ===\r\n"));
						//closesocket(msock);
						//WSACleanup();
						//return TRUE;
					//}


					break;
				case FD_WRITE:
				//Write your code for write event here

					break;
				case FD_CLOSE:
					break;
			};
			break;


		case WM_SOCKET_NOTIFY_C0:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_CONNECT:
					Mystatus[0] = F_WRITING;
					//EditPrintf(hwndEdit, TEXT("=== connect to server0 ===\r\n"));
					err = WSAAsyncSelect(client_fd[0], hwnd, WM_SOCKET_NOTIFY_C0, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					break;
				case FD_READ:
				//Write your code for read event here.

					

					EditPrintf(hwndEdit, TEXT("=== no.0 read  ===\r\n"));
					
					len = recv_msg(0,client_fd[0],hwndEdit,hwnd);

					if(Mystatus[0] == F_WRITING)
						err = WSAAsyncSelect(client_fd[0], hwnd, WM_SOCKET_NOTIFY_C0, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					
					//EditPrintf(hwndEdit, TEXT("===  read len is %d  ===\r\n"),len);
					//Sleep(2000);

					if(len<=0){
						//read finished
						EditPrintf(hwndEdit, TEXT("=== no.0 read finished ===\r\n"));
						//FD_CLR(client_fd[i],&rs);
						Mystatus[0] = F_DONE;
						conn--;
					}
					
					
					


					break;
				case FD_WRITE:
					//Write your code for write event here

					if(Mystatus[0] != F_WRITING)
						break;
					
					//read from file
					//EditPrintf(hwndEdit, TEXT("=== no.0 write ===\r\n"));
					
					len = readline(filefd[0],buffer,BUFSIZE);
					if(len < 0)
						EditPrintf(hwndEdit, TEXT("=== readline failed! ===\r\n"));
			


					buffer[len-1] = 13;
					buffer[len] = 10;
					buffer[len+1] = '\0';
					//EditPrintf(hwndEdit, TEXT("readline is %s\r\n"),buffer);
					
					//fflush(stdout);
					//printf("(no.%d) command:%s",i,msg_buf);
					//write(1,buf,strlen(msg_buf1)+1);
					//printf("%s",msg_buf1);
					fflush(stdout);
					//len = write(client_fd[i],buffer,BUFSIZE);
					len = send(client_fd[0],buffer,strlen(buffer),0);
					//NeedWrite[i] -= AlreadyWrite[i];
					//FD_SET(client_fd[i],&rs);
					Mystatus[0] = F_READING;	
				
				
				
				//eliminate '\r'
					check = buffer;
					while(*check != '\0'){
						if(check[0] == '\r')
							check[0] = '\0';
						check++;
				
					}


					//printf("<script>document.all['m0'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);

					sprintf(commandbuffer,"<script>document.all['m0'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);
					EditPrintf(hwndEdit, TEXT("=== wrtie string id %s ===\r\n"), commandbuffer);
					send(ssock,commandbuffer,strlen(commandbuffer),0);
					fflush(stdout);
					
					//Sleep(3000);

					//if(AlreadyWrite[i]<=0 || NeedWrite[i]<=0){
					if(!strncmp(buffer,"exit",4)){
						//write finished
						//printf("no.%d write finished!\n",i);
						//FD_CLR(client_fd[i],&ws);
						Mystatus[0] = F_READING;
						//FD_SET(client_fd[i],&rs);
					}
					
					break;
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SOCKET_NOTIFY_C1:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_CONNECT:
					Mystatus[1] = F_WRITING;
					EditPrintf(hwndEdit, TEXT("=== connect to server0 ===\r\n"));
					err = WSAAsyncSelect(client_fd[1], hwnd, WM_SOCKET_NOTIFY_C1, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					break;
				case FD_READ:
				//Write your code for read event here.

					

					//EditPrintf(hwndEdit, TEXT("=== no.0 read  ===\r\n"));
					
					len = recv_msg(1,client_fd[1],hwndEdit,hwnd);
					
					if(Mystatus[1] == F_WRITING)
						err = WSAAsyncSelect(client_fd[1], hwnd, WM_SOCKET_NOTIFY_C1, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					
					//EditPrintf(hwndEdit, TEXT("===  read len is %d  ===\r\n"),len);
					//Sleep(2000);

					if(len<=0){
						//read finished
						EditPrintf(hwndEdit, TEXT("=== no.0 read finished ===\r\n"));
						//FD_CLR(client_fd[i],&rs);
						Mystatus[1] = F_DONE;
						conn--;
					}
					
					
					


					break;
				case FD_WRITE:
					//Write your code for write event here

					if(Mystatus[1] != F_WRITING)
						break;
					
					//read from file
					//EditPrintf(hwndEdit, TEXT("=== no.0 write ===\r\n"));
					
					len = readline(filefd[1],buffer,BUFSIZE);
					if(len < 0)
						EditPrintf(hwndEdit, TEXT("=== readline failed! ===\r\n"));
			


					buffer[len-1] = 13;
					buffer[len] = 10;
					buffer[len+1] = '\0';
					//EditPrintf(hwndEdit, TEXT("readline is %s\r\n"),buffer);
					
					//fflush(stdout);
					//printf("(no.%d) command:%s",i,msg_buf);
					//write(1,buf,strlen(msg_buf1)+1);
					//printf("%s",msg_buf1);
					fflush(stdout);
					//len = write(client_fd[i],buffer,BUFSIZE);
					len = send(client_fd[1],buffer,strlen(buffer),0);
					//NeedWrite[i] -= AlreadyWrite[i];
					//FD_SET(client_fd[i],&rs);
					Mystatus[1] = F_READING;	
				
				
				
				//eliminate '\r'
					check = buffer;
					while(*check != '\0'){
						if(check[0] == '\r')
							check[0] = '\0';
						check++;
				
					}


					//printf("<script>document.all['m0'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);

					sprintf(commandbuffer,"<script>document.all['m1'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);
					EditPrintf(hwndEdit, TEXT("=== wrtie string id %s ===\r\n"), commandbuffer);
					send(ssock,commandbuffer,strlen(commandbuffer),0);
					fflush(stdout);
					
					//Sleep(3000);

					//if(AlreadyWrite[i]<=0 || NeedWrite[i]<=0){
					if(!strncmp(buffer,"exit",4)){
						//write finished
						//printf("no.%d write finished!\n",i);
						//FD_CLR(client_fd[i],&ws);
						Mystatus[1] = F_READING;
						//FD_SET(client_fd[i],&rs);
					}
					
					break;
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SOCKET_NOTIFY_C2:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_CONNECT:
					Mystatus[2] = F_WRITING;
					EditPrintf(hwndEdit, TEXT("=== connect to server0 ===\r\n"));
					err = WSAAsyncSelect(client_fd[2], hwnd, WM_SOCKET_NOTIFY_C2, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					break;
				case FD_READ:
				//Write your code for read event here.

					

					//EditPrintf(hwndEdit, TEXT("=== no.0 read  ===\r\n"));
					
					len = recv_msg(2,client_fd[2],hwndEdit,hwnd);
					
					if(Mystatus[2] == F_WRITING)
						err = WSAAsyncSelect(client_fd[2], hwnd, WM_SOCKET_NOTIFY_C2, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					
					//EditPrintf(hwndEdit, TEXT("===  read len is %d  ===\r\n"),len);
					//Sleep(2000);

					if(len<=0){
						//read finished
						EditPrintf(hwndEdit, TEXT("=== no.0 read finished ===\r\n"));
						//FD_CLR(client_fd[i],&rs);
						Mystatus[2] = F_DONE;
						conn--;
					}
					
					
					


					break;
				case FD_WRITE:
					//Write your code for write event here

					if(Mystatus[2] != F_WRITING)
						break;
					
					//read from file
					//EditPrintf(hwndEdit, TEXT("=== no.0 write ===\r\n"));
					
					len = readline(filefd[2],buffer,BUFSIZE);
					if(len < 0)
						EditPrintf(hwndEdit, TEXT("=== readline failed! ===\r\n"));
			


					buffer[len-1] = 13;
					buffer[len] = 10;
					buffer[len+1] = '\0';
					//EditPrintf(hwndEdit, TEXT("readline is %s\r\n"),buffer);
					
					//fflush(stdout);
					//printf("(no.%d) command:%s",i,msg_buf);
					//write(1,buf,strlen(msg_buf1)+1);
					//printf("%s",msg_buf1);
					fflush(stdout);
					//len = write(client_fd[i],buffer,BUFSIZE);
					len = send(client_fd[2],buffer,strlen(buffer),0);
					//NeedWrite[i] -= AlreadyWrite[i];
					//FD_SET(client_fd[i],&rs);
					Mystatus[2] = F_READING;	
				
				
				
				//eliminate '\r'
					check = buffer;
					while(*check != '\0'){
						if(check[0] == '\r')
							check[0] = '\0';
						check++;
				
					}


					//printf("<script>document.all['m0'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);

					sprintf(commandbuffer,"<script>document.all['m2'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);
					EditPrintf(hwndEdit, TEXT("=== wrtie string id %s ===\r\n"), commandbuffer);
					send(ssock,commandbuffer,strlen(commandbuffer),0);
					fflush(stdout);
					
					//Sleep(3000);

					//if(AlreadyWrite[i]<=0 || NeedWrite[i]<=0){
					if(!strncmp(buffer,"exit",4)){
						//write finished
						//printf("no.%d write finished!\n",i);
						//FD_CLR(client_fd[i],&ws);
						Mystatus[2] = F_READING;
						//FD_SET(client_fd[i],&rs);
					}
					
					break;
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SOCKET_NOTIFY_C3:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_CONNECT:
					Mystatus[3] = F_WRITING;
					EditPrintf(hwndEdit, TEXT("=== connect to server0 ===\r\n"));
					err = WSAAsyncSelect(client_fd[3], hwnd, WM_SOCKET_NOTIFY_C3, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					break;
				case FD_READ:
				//Write your code for read event here.

					

					//EditPrintf(hwndEdit, TEXT("=== no.0 read  ===\r\n"));
					
					len = recv_msg(3,client_fd[3],hwndEdit,hwnd);
					
					if(Mystatus[3] == F_WRITING)
						err = WSAAsyncSelect(client_fd[3], hwnd, WM_SOCKET_NOTIFY_C3, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					
					//EditPrintf(hwndEdit, TEXT("===  read len is %d  ===\r\n"),len);
					//Sleep(2000);

					if(len<=0){
						//read finished
						EditPrintf(hwndEdit, TEXT("=== no.0 read finished ===\r\n"));
						//FD_CLR(client_fd[i],&rs);
						Mystatus[3] = F_DONE;
						conn--;
					}
					
					
					


					break;
				case FD_WRITE:
					//Write your code for write event here

					if(Mystatus[3] != F_WRITING)
						break;
					
					//read from file
					//EditPrintf(hwndEdit, TEXT("=== no.0 write ===\r\n"));
					
					len = readline(filefd[3],buffer,BUFSIZE);
					if(len < 0)
						EditPrintf(hwndEdit, TEXT("=== readline failed! ===\r\n"));
			


					buffer[len-1] = 13;
					buffer[len] = 10;
					buffer[len+1] = '\0';
					//EditPrintf(hwndEdit, TEXT("readline is %s\r\n"),buffer);
					
					//fflush(stdout);
					//printf("(no.%d) command:%s",i,msg_buf);
					//write(1,buf,strlen(msg_buf1)+1);
					//printf("%s",msg_buf1);
					fflush(stdout);
					//len = write(client_fd[i],buffer,BUFSIZE);
					len = send(client_fd[3],buffer,strlen(buffer),0);
					//NeedWrite[i] -= AlreadyWrite[i];
					//FD_SET(client_fd[i],&rs);
					Mystatus[3] = F_READING;	
				
				
				
				//eliminate '\r'
					check = buffer;
					while(*check != '\0'){
						if(check[0] == '\r')
							check[0] = '\0';
						check++;
				
					}


					//printf("<script>document.all['m0'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);

					sprintf(commandbuffer,"<script>document.all['m3'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);
					EditPrintf(hwndEdit, TEXT("=== wrtie string id %s ===\r\n"), commandbuffer);
					send(ssock,commandbuffer,strlen(commandbuffer),0);
					fflush(stdout);
					
					//Sleep(3000);

					//if(AlreadyWrite[i]<=0 || NeedWrite[i]<=0){
					if(!strncmp(buffer,"exit",4)){
						//write finished
						//printf("no.%d write finished!\n",i);
						//FD_CLR(client_fd[i],&ws);
						Mystatus[3] = F_READING;
						//FD_SET(client_fd[i],&rs);
					}
					
					break;
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SOCKET_NOTIFY_C4:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_CONNECT:
					Mystatus[4] = F_WRITING;
					EditPrintf(hwndEdit, TEXT("=== connect to server0 ===\r\n"));
					err = WSAAsyncSelect(client_fd[4], hwnd, WM_SOCKET_NOTIFY_C4, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					break;
				case FD_READ:
				//Write your code for read event here.

					

					//EditPrintf(hwndEdit, TEXT("=== no.0 read  ===\r\n"));
					
					len = recv_msg(4,client_fd[4],hwndEdit,hwnd);
					
					if(Mystatus[4] == F_WRITING)
						err = WSAAsyncSelect(client_fd[4], hwnd, WM_SOCKET_NOTIFY_C4, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
					
					//EditPrintf(hwndEdit, TEXT("===  read len is %d  ===\r\n"),len);
					//Sleep(2000);

					if(len<=0){
						//read finished
						EditPrintf(hwndEdit, TEXT("=== no.0 read finished ===\r\n"));
						//FD_CLR(client_fd[i],&rs);
						Mystatus[4] = F_DONE;
						conn--;
					}
					
					
					


					break;
				case FD_WRITE:
					//Write your code for write event here

					if(Mystatus[4] != F_WRITING)
						break;
					
					//read from file
					//EditPrintf(hwndEdit, TEXT("=== no.0 write ===\r\n"));
					
					len = readline(filefd[4],buffer,BUFSIZE);
					if(len < 0)
						EditPrintf(hwndEdit, TEXT("=== readline failed! ===\r\n"));
			
					

					buffer[len-1] = 13;
					buffer[len] = 10;
					buffer[len+1] = '\0';
					//EditPrintf(hwndEdit, TEXT("readline is %s\r\n"),buffer);
					
					//fflush(stdout);
					//printf("(no.%d) command:%s",i,msg_buf);
					//write(1,buf,strlen(msg_buf1)+1);
					//printf("%s",msg_buf1);
					fflush(stdout);
					//len = write(client_fd[i],buffer,BUFSIZE);
					len = send(client_fd[4],buffer,strlen(buffer),0);
					//NeedWrite[i] -= AlreadyWrite[i];
					//FD_SET(client_fd[i],&rs);
					Mystatus[4] = F_READING;	
				
				
					
				//eliminate '\r'
					check = buffer;
					while(*check != '\0'){
						if(check[0] == '\r')
							check[0] = '\0';
						check++;
				
					}


					//printf("<script>document.all['m0'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);

					sprintf(commandbuffer,"<script>document.all['m4'].innerHTML += \" <b>%s</b><br>\";</script>\n",buffer);
					EditPrintf(hwndEdit, TEXT("=== wrtie string id %s ===\r\n"), commandbuffer);
					send(ssock,commandbuffer,strlen(commandbuffer),0);
					fflush(stdout);
					
					//Sleep(3000);

					//if(AlreadyWrite[i]<=0 || NeedWrite[i]<=0){
					if(!strncmp(buffer,"exit",4)){
						//write finished
						//printf("no.%d write finished!\n",i);
						//FD_CLR(client_fd[i],&ws);
						Mystatus[4] = F_READING;
						//FD_SET(client_fd[i],&rs);
					}

					
					break;
				case FD_CLOSE:
					break;
			};

			break;
		
		default:
			return FALSE;


	};

	return TRUE;
}

int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}

void handle_socket(int fd,static HWND hwnd)
{
    /*int j, file_fd, buflen, len;
	unsigned long val;
    long i, ret;
    char * fstr;
	int status;
	FILE *file;
	struct hostent *he;
	int    client_fd[MAXUSER];
    struct sockaddr_in client_sin[MAXUSER];
	char *input_argv[MAX_ARGV_NUM];
	char *input[MAX_ARGV_NUM];
	int conn=0;
	int input_argc = 0;
	int num_server = -1;
	char *batch_file[MAXUSER];
	int  port[MAXUSER];
	char *hostname[MAXUSER];
	FILE *filefd[MAXUSER];
	int Mystatus[MAXUSER];
	*/

	
	char index[] = 

	"\
	<html>\n\
	<head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://java.csie.nctu.edu.tw/course/theme/standard/styles.php\" />\n\
	<link rel=\"stylesheet\" type=\"text/css\" href=\"http://java.csie.nctu.edu.tw/course/theme/standardwhite/styles.php\" />\n\
	<link rel=\"stylesheet\" type=\"text/css\" href=\"http://java.csie.nctu.edu.tw/course/theme/standard/styles.php\" />\n\
	<link rel=\"stylesheet\" type=\"text/css\" href=\"http://java.csie.nctu.edu.tw/course/theme/standardwhite/styles.php\" />\n\
	<link rel=\"stylesheet\" type=\"text/css\" href=\"http://java.csie.nctu.edu.tw/course/theme/standard/styles.php\" />\n\
	<link rel=\"stylesheet\" type=\"text/css\" href=\"http://java.csie.nctu.edu.tw/course/theme/standardwhite/styles.php\" />\n\
	<title>Network Programming Homework3</title>\n\
	</head>\n\
	<body bgcolor=\"#336699\">\n\
	<form method=\"get\" action=\"hw3.cgi\">\n\
	<table>\n\
	<tr><td>&nbsp;</td><td>IP</td><td>PORT</td><td>Patch File Name</td></tr>\n\
	<tr><td>Host1</td><td><input type=\"text\" name=\"h1\"></td><td><input type=\"text\" name=\"p1\"></td><td><input type=\"text\" name=\"f1\"></td></tr>\n\
	<tr><td>Host2</td><td><input type=\"text\" name=\"h2\"></td><td><input type=\"text\" name=\"p2\"></td><td><input type=\"text\" name=\"f2\"></td></tr>\n\
	<tr><td>Host3</td><td><input type=\"text\" name=\"h3\"></td><td><input type=\"text\" name=\"p3\"></td><td><input type=\"text\" name=\"f3\"></td></tr>\n\
	<tr><td>Host4</td><td><input type=\"text\" name=\"h4\"></td><td><input type=\"text\" name=\"p4\"></td><td><input type=\"text\" name=\"f4\"></td></tr>\n\
	<tr><td>Host5</td><td><input type=\"text\" name=\"h5\"></td><td><input type=\"text\" name=\"p5\"></td><td><input type=\"text\" name=\"f5\"></td></tr>\n\
	<tr><td colspan=\"5\"><input type=\"submit\" value=\"Send\"></td></tr>\n\
	</table>\n\
	</form>\n\
	</body>\n\
	</html>\0" ;

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


	//struct stat file_stat;
	//int NeedWrite;

	

    ret = recv(fd,buffer,BUFSIZE,0);   
    if (ret==0||ret==-1) {
     
        exit(3);
    }

    
    if (ret>0&&ret<BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;

    
	strcpy(bufferParse,buffer);
	//printf("nByteRead:  Recv message from client:\n%s", buffer);
	EditPrintf(hwnd, TEXT("nByteRead:  Recv message from client:\n%s\r\n"), buffer);


	

    if (strncmp(buffer,"GET ",4)&&strncmp(buffer,"get ",4))
        exit(3);
    

	
    if (!strncmp(&buffer[0],"GET / HTTP\0",10)||!strncmp(&buffer[0],"get / http\0",10) ){
        
		strcpy(buffer,"form_get.htm");

		//stat(buffer,&file_stat);
		//NeedWrite = file_stat.st_size;

		file = fopen(buffer,"r");
		
			//send(fd, "Failed to open file", 19,0);
		
		sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: \r\n\r\n");
		send(fd,buffer,strlen(buffer),0);

		len=fread(buffer,sizeof(char),BUFSIZE,file);
		//EditPrintf(hwnd, TEXT("\nhaha is%s\r\n"), buffer);
		
		send(fd,buffer,len,0);
		closesocket(fd);
		//strcpy(buffer,"form_get.htm");
		//SendFile(fd,buffer);

	
	}
	else
	{
		
		input_argc = 0;
		num_server = -1;
		
		
		parse_env(buffer);
		EditPrintf(hwnd, TEXT("query_string:::\n%s\r\n"), query_string);

		//send index.html
		sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: \r\n\r\n");
		send(fd,buffer,strlen(buffer),0);
		strcpy(buffer,"index.htm");

		//stat(buffer,&file_stat);
		//NeedWrite = file_stat.st_size;

		file = fopen(buffer,"r");
		len=fread(buffer,sizeof(char),BUFSIZE,file);
		//EditPrintf(hwnd, TEXT("\nhaha is%s\r\n"), buffer);
		
		//send(fd,buffer,len,0);
		
		send(fd,cgi_text,strlen(cgi_text),0);
		//closesocket(fd);


		// parse the QUERY_STRING
		input_argv[input_argc] = strtok(query_string,"&");
		input_argv[input_argc][strlen(input_argv[input_argc])] = '\0';

		while(input_argv[++input_argc] = strtok(NULL,"&"))
			input_argv[input_argc][strlen(input_argv[input_argc])] = '\0' ;
	
	
	
		//printf("input_argc is %d\n",input_argc);
		fflush(stdout);

		for(i=0;i<input_argc;i++){
			//printf("input_argv[%d] is %s\n",i,input_argv[i]);
			EditPrintf(hwnd, TEXT("input_argv[%d] is %s\n"),i,input_argv[i]);
			
			if(input_argv[i][0] == 'h' && (input_argv[i][3] != '\0' && input_argv[i][3] != ' ')){
				num_server++;
				hostname[num_server] = input_argv[i]+3;
				sprintf(commandbuffer,"<script>document.all['a%d'].innerHTML += \" <b>%s</b><br>\";</script>\n",num_server,hostname[num_server]);
				EditPrintf(hwnd, TEXT("hostname[%d] is %s\r\n"), num_server,hostname[num_server]);
				
				send(ssock,commandbuffer,strlen(commandbuffer),0);
				//printf("  hostname[%d] is %s\n",num_server,hostname[num_server]);
			}
			if(input_argv[i][0] == 'p' && (input_argv[i][3] != '\0' && input_argv[i][3] != ' ')){
				port[num_server] = atoi(input_argv[i]+3);
				EditPrintf(hwnd, TEXT("port[%d] is %d\r\n"), num_server,port[num_server]);
				//printf("  port[%d] is %d\n",num_server,port[num_server]);
			}
			if(input_argv[i][0] == 'f' && (input_argv[i][3] != '\0' && input_argv[i][3] != ' ' && input_argv[i][3] != 'H')){
				batch_file[num_server]=input_argv[i]+3;
				EditPrintf(hwnd, TEXT("batch_file[%d] is %s\r\n"), num_server,batch_file[num_server]);
				//printf("  batch_file[%d] is %s\n",num_server,batch_file[num_server]);

			}
			

		}
		//printf("num_server is %d\n",num_server);

		conn = num_server+1;

		/*file = fopen(batch_file[0],"r");
		len = readline(file,buffer,BUFSIZE);
		buffer[len-1] = 13;
		buffer[len] = 10;
		buffer[len+1] = '\0';

		EditPrintf(hwnd, TEXT("readline is %s\r\n"),buffer);
		len = readline(file,buffer,BUFSIZE);
		buffer[len-1] = 13;
		buffer[len] = 10;
		buffer[len+1] = '\0';

		EditPrintf(hwnd, TEXT("readlineer is %s\r\n"),buffer);
		*/
		
		for(i=0;i<conn;i++)
		{
        
			he = gethostbyname(hostname[i]);

			filefd[i] = fopen(batch_file[i],"r");
			
			//stat(batch_file[i],&file_stat);
			//NeedWrite[i] = file_stat.st_size;
			//printf("%s's size is %d\n",batch_file1,NeedWrite[i]);

			client_fd[i] = socket(AF_INET,SOCK_STREAM,0);
			//bzero(&client_sin[i],sizeof(client_sin[i]));
			memset(&client_sin[i],'\0', sizeof(client_sin[i]));
			client_sin[i].sin_family = AF_INET;
			client_sin[i].sin_addr = *((struct in_addr *)he->h_addr); 
			client_sin[i].sin_port = htons(port[i]);

			val = 1;
			ioctlsocket( client_fd[i], FIONBIO, &val);

			//printf("sockfd[%d] is %d\n",i,client_fd[i]);
			if(connect(client_fd[i],(struct sockaddr *)&client_sin[i],sizeof(client_sin[i])) < 0){
				//printf("connection wrong\n");
				//if(errno != EINPROGRESS){
					printf("connection failed\n");
					fflush(stdout);
					//return -1;
			
				//}
    		}
			//Sleep(1000);
			//FD_SET(client_fd[i],&rs);
			//FD_SET(client_fd[i],&ws);
			//FD_SET(file_fd[i],&rs);
			
			Mystatus[i] = F_CONNECTING;//connecting	
		
		}

		

		
		
		

		

		
			
	
	
	
	
	}

}

char *mySavestring (char *ptr, size_t size)
{
    char *p = (char *) malloc (size + 1);
    memcpy (p, ptr, size);
    p[size] = 0;
    return p;
}

void parse_env(char *msg){
	
	char *begin;
	char *end;
	char *queryString;
	char *env;
	char *tmp = (char*)malloc((strlen(msg)+1)*sizeof(char));
	strcpy(tmp,msg);
	
	//QUERY_STRING starts from '?'
	begin = tmp + strcspn(tmp,"?");
	begin++;
	
	end = tmp + strcspn(tmp,"H");

	queryString = mySavestring(begin,end-begin);
	strcpy(query_string,queryString);
	
	
}

BOOL SendFile(int s, char* fname)
{	
    FILE* f;
    f=fopen(fname,"r");	
    char buff[1024];
    int y;
    int x;
    if(!f)
        return false;
    while(true)
    {		
        //y=fread(buff,1024);
		y=fread(buff,sizeof(char),1024,f);
        x=send(s,buff,y,0);		
        if(y<1024)
        {
            fclose(f);
            break;
        }			
    }	

    return true;
}

int readline(FILE* fd,char *ptr,int maxlen)
{
        int n,rc;
	char c;
        *ptr = 0;
        for(n=1;n<maxlen;n++)
        {
        	if((rc=fread(&c,sizeof(char),1,fd)) == 1)
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

int recv_msg(int userno,int from,static HWND hwndEdit,static HWND hwnd)
{
    char buf[3000],*tmp,*a;
	char sendbuf[3000];
    int len,i,err;
    if((len=recv(from,buf,sizeof(buf)-1,0)) <0) 
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

				sprintf(sendbuf,"<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n",userno,tmp);
				EditPrintf(hwndEdit, TEXT("=== send string id %s ===\r\n"), sendbuf);
				send(ssock,sendbuf,strlen(sendbuf),0);
				fflush(stdout);
			//}
			
			
			if(tmp[0] == '%'){
				Mystatus[userno] = F_WRITING;
				//err = WSAAsyncSelect(client_fd[0], hwnd, WM_SOCKET_NOTIFY_C0, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
			}
        }
    }
    fflush(stdout); 
    return len;
}