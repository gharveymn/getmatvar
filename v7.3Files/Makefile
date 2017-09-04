CC=clang
CFLAGS=-c -g -Wall

all: mapping

mapping: userTest.o mapping.o fileHelper.o numberHelper.o queue.o getPageSize.o readMessage.o chunkedData.o
	$(CC) userTest.o mapping.o fileHelper.o numberHelper.o queue.o getPageSize.o readMessage.o chunkedData.o -o mapping -lm -lz

userTest.o: userTest.c
	$(CC) $(CFLAGS) userTest.c

mapping.o: mapping.c
	$(CC) $(CFLAGS) mapping.c

fileHelper.o: fileHelper.c
	$(CC) $(CFLAGS) fileHelper.c

numberHelper.o: numberHelper.c
	$(CC) $(CFLAGS) numberHelper.c

queue.o: queue.c
	$(CC) $(CFLAGS) queue.c

getPageSize.o: getPageSize.c
	$(CC) $(CFLAGS) getPageSize.c

readMessage.o: readMessage.c
	$(CC) $(CFLAGS) readMessage.c

chunkedData.o: chunkedData.c
	$(CC) $(CFLAGS) chunkedData.c

clean: 
	rm *o mapping