server : server.o 
	gcc -o server server.o -lpthread -g
server.o : server.c server.h list.h
	gcc -g -c server.c
clean :
	rm -f *.o
