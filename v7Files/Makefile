CC=clang
CFLAGS=-I. -g -Wall
DEPS = mapping.h
OBJ = chunkedData.o adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o zutil.o inflate.o infback.o inftrees.o inffast.o mapping.o fileHelper.o numberHelper.o queue.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

chunkedData: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lm
