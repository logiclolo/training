client server:	TCPSocketBlockClient.o TCPSocketBlockServer.o 
	gcc -Wall -o client TCPSocketBlockClient.o 
	gcc -Wall -o server TCPSocketBlockServer.o 

TCPSocketBlockClient.o:	TCPSocketBlockClient.c 
	gcc -Wall -c TCPSocketBlockClient.c 

TCPSocketBlockServer.o:	TCPSocketBlockServer.c 
	gcc -Wall -c TCPSocketBlockServer.c 

.PHONY: clean

clean:
	rm -f client server TCPSocketBlockClient.o TCPSocketBlockServer.o 
