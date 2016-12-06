client server: MulticastServer.o MulticastClient.o
	gcc -Wall -o client MulticastClient.o
	arm-linux-gcc -Wall -o server MulticastServer.o

MulticastServer.o: MulticastServer.c
	arm-linux-gcc -Wall -c MulticastServer.c

MulticastClient.o: MulticastClient.c
	gcc -Wall -c MulticastClient.c

.PHONY:clean

clean:
	rm -f client server MulticastServer.o MulticastClient.o
