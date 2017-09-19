CC=gcc

LIBDEFLATE_INC=./src/extlib/libdeflate/x64
#LIBDEFLATE_INC=./src/libdeflate
THREADPOOL_INC=./src/extlib/threadpool

LIBDEFLATE_LIB=./src/extlib/libdeflate/x64

INC=$(LIBDEFLATE_INC) $(THREADPOOL_INC)
INC_PARAMS=$(foreach d, $(INC), -I$d)

ODIR=src

#LIB=$(LIBDEFLATE_LIB)
LIB=.
LIB_PARAMS=$(foreach d, $(LIB), -I$d)

LIBS=-lm -pthread

_DEPS = mapping.h libdeflate.h threadpool.h queue.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = ../tests/memtest.o mapping.o fileHelper.o \
numberHelper.o queue.o getSystemInfo.o \
readMessage.o chunkedData.o \
extlib/threadpool/thpool.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

CFLAGS=-g -Wall --std=c99 $(INC_PARAMS) $(LIB_PARAMS)

all: mapping

mapping: $(OBJ) src/extlib/libdeflate/x64/libdeflate.a
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

src/libdeflate/libdeflate.a:
	cd $(ODIR)/libdeflate/ && $(MAKE)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
