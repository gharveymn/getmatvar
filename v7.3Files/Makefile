CC=clang
CFLAGS=-c -g -Wall

all: mapping

mapping: mapping.o fileHelper.o numberHelper.o queue.o
	$(CC) mapping.o fileHelper.o numberHelper.o queue.o -o mapping -lm

mapping.o: mapping.c
	$(CC) $(CFLAGS) mapping.c

fileHelper.o: fileHelper.c
	$(CC) $(CFLAGS) fileHelper.c

numberHelper.o: numberHelper.c
	$(CC) $(CFLAGS) numberHelper.c

queue.o: queue.c
	$(CC) $(CFLAGS) queue.c

clean: 
	rm *o mapping