client server: TCPSocketNonblockClient.o TCPSocketNonblockServer.o
	gcc -Wall -o client TCPSocketNonblockClient.o
	gcc -Wall -o server TCPSocketNonblockServer.o

TCPSocketNonblockClient.o: TCPSocketNonblockClient.c
	gcc -Wall -c TCPSocketNonblockClient.c

TCPSocketNonblockServer.o: TCPSocketNonblockServer.c
	gcc -Wall -c TCPSocketNonblockServer.c

.PHONY:clean

clean:
	rm -f client server TCPSocketNonblockClient.o TCPSocketNonblockServer.o
