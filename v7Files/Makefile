CC=gcc
CFLAGS = -g -o
DEPS= decompressTest.c adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c trees.c \
       zutil.c inflate.c infback.c inftrees.c inffast.c

decompressTest: $(DEPS)
	$(CC) $(CFLAGS) decompressTest $(DEPS) -I.
