client: TCPSocketGetIP.o
	gcc -Wall -o client TCPSocketGetIP.o

TCPSocketGetIP.o: TCPSocketGetIP.c
	gcc -Wall -c TCPSocketGetIP.c

.PHONY:clean

clean:
	rm -f client TCPSocketGetIP.o
