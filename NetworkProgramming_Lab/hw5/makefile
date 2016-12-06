client: TCPSocketTimeoutClient.o
	gcc -Wall -o client TCPSocketTimeoutClient.o

TCPSocketTimeoutClient.o: TCPSocketTimeoutClient.c
	gcc -Wall -c TCPSocketTimeoutClient.c

.PHONY:clean

clean:
	rm -f TCPSocketTimeoutClient.o client
