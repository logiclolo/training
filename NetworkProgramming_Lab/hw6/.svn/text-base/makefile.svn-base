client broadcast: broadcastClient.o broadcast.o
	gcc -Wall -o client broadcastClient.o
	gcc -Wall -o broadcast broadcast.o

broadcastClient.o: broadcastClient.c
	gcc -Wall -c broadcastClient.c

broadcast.o: broadcast.c
	gcc -Wall -c broadcast.c

.PHONY:clean

clean:
	rm -f client broadcast broadcast.o broadcastClient.o
