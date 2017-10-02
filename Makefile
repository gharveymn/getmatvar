CC=gcc

#===DIRECTORIES===#
OBJ_DIR=src
SRC_DIR=src
TST_DIR=tests


LIBDEFLATE_INC=./src/extlib/libdeflate/x64/unix
THREADPOOL_INC=./src/extlib/thpool

LIBDEFLATE_LIB=./src/extlib/libdeflate/x64/unix

INC=$(LIBDEFLATE_INC) $(THREADPOOL_INC)
INC_PARAMS=$(foreach d, $(INC), -I$d)


#LIB=$(LIBDEFLATE_LIB)
LIB=.
LIB_PARAMS=$(foreach d, $(LIB), -I$d)

LIBS=-lm -lpthread

_DEPS = mapping.h libdeflate.h thpool.h queue.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = ../$(TST_DIR)/mexemulate.o \
cleanup.o \
createDataObjects.o \
ezq.o \
fillDataObjects.o \
getDataObjects.o \
getSystemInfo.o \
init.o \
navigate.o \
numberHelper.o \
placeChunkedData.o \
placeData.o \
readMessage.o \
superblock.o \
utils.o \
extlib/thpool/thpool.o \

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))


$(OBJ_DIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

CFLAGS=-g -Wall --std=c99 -DNO_MEX $(INC_PARAMS) $(LIB_PARAMS)

all: mapping

mapping: $(OBJ) src/extlib/libdeflate/x64/unix/libdeflate.a
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

src/extlib/libdeflate/x64/unix/libdeflate.a:
	cd $(OBJ_DIR)/extlib/libdeflate/x64/unix && $(MAKE)

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o $(TST_DIR)/*.o *~ core $(INCDIR)/*~
