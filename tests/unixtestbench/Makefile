CC=gcc

#===DIRECTORIES===#
OBJ_DIR=src
SRC_DIR=src


LIBDEFLATE_INC=./src/extlib/libdeflate/x64
#LIBDEFLATE_INC=./src/libdeflate
THREADPOOL_INC=./src/extlib/thpool

LIBDEFLATE_LIB=./src/extlib/libdeflate/x64

INC=$(LIBDEFLATE_INC) $(THREADPOOL_INC)
INC_PARAMS=$(foreach d, $(INC), -I$d)


#LIB=$(LIBDEFLATE_LIB)
LIB=.
LIB_PARAMS=$(foreach d, $(LIB), -I$d)

LIBS=-lm -pthread

_DEPS = mapping.h libdeflate.h thpool.h queue.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = ../tests/memtest.o mapping.o fileHelper.o \
numberHelper.o queue.o getSystemInfo.o \
readMessage.o chunkedData.o \
extlib/thpool/thpool.o

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))


$(OBJ_DIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

CFLAGS=-g -Wall --std=c99 $(INC_PARAMS) $(LIB_PARAMS)

all: mapping

mapping: $(OBJ) src/extlib/libdeflate/x64/libdeflate.a
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

src/libdeflate/libdeflate.a:
	cd $(OBJ_DIR)/libdeflate/ && $(MAKE)

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o *~ core $(INCDIR)/*~
