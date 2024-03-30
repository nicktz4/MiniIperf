
CC = gcc
CFLAGS = -Wall -pedantic

EXEC = miniIperf
SOURCES = $(*.c)
HEADERS = $(*.h)
OBJECTS = ${SOURCES:%.c=obj/%.o}
IP = 

default: $(EXEC)





miniIperf:  main.o client.o server.o utils.o miniIperf_time.o
	$(CC) $(CFLAGS) $^ -o $(EXEC) -lm


miniIperfClient1: miniIperf
	./$(EXEC) -c -a $(IP)  -b 1000000  -t 100

miniIperfClient2: miniIperf
	./$(EXEC) -c -a $(IP)  -b 10000000  -t 2

miniIperfClient3: miniIperf
	./$(EXEC) -c -a $(IP)  -b 10000000  -t 8

miniIperfClient4: miniIperf
	./$(EXEC) -c -a $(IP)    -t 20
miniIperfServer1: miniIperf 
	./$(EXEC)  -s -i 1 -a $(IP)
miniIperfServer2: miniIperf 
	./$(EXEC)  -s -i 2 -a $(IP)   
miniIperfServer3: miniIperf 
	./$(EXEC)  -s -i 2 -a $(IP) -f output.txt   





%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $< 

clean:
	-rm -f *.o
	-rm $(EXEC)
	-rm output.txt