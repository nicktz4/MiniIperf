
CC = gcc
CFLAGS = -Wall -pedantic

EXEC = miniIperf
SOURCES = $(*.c)
HEADERS = $(*.h)
OBJECTS = ${SOURCES:%.c=obj/%.o}
IP = 192.168.1.31

default: $(EXEC)


miniIperf: main.o client.o server.o utils.o
	$(CC) $(CFLAGS) $^ -o $(EXEC)


miniIperfClient: miniIperf
	./$(EXEC) -c -a $(IP)
miniIperfServer: miniIperf 
	./$(EXEC) -s -i 2 -a $(IP)


%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f *.o
	-rm $(EXEC)