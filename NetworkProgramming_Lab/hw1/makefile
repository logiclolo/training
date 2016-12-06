client server:	UDPclient.o UDPserver.o 
	gcc -Wall -o client UDPclient.o
	gcc -Wall -o server UDPserver.o

UDPclient.o: UDPclient.c
	gcc -Wall -c UDPclient.c

UDPserver.o: UDPserver.c
	gcc -Wall -c UDPserver.c

.PHONY: clean

clean:
	rm -f client server UDPclient.o UDPserver.o
