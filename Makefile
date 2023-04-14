
CC = gcc
CFLAGS = -Wall -pedantic

EXEC = miniIperf
SOURCES = $(*.c)
HEADERS = $(*.h)
OBJECTS = ${SOURCES:%.c=obj/%.o}
IP = 192.168.1.31

default: $(EXEC)





miniIperf:  main.o client.o server.o utils.o miniIperf_time.o
	$(CC) $(CFLAGS) $^ -o $(EXEC) -lm


miniIperfClient: miniIperf
	./$(EXEC) -c -a $(IP)  -b 1000000  
miniIperfServer: miniIperf 
	./$(EXEC) -s -i 2 -a $(IP)  


%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $< 

clean:
	-rm -f *.o
	-rm $(EXEC)