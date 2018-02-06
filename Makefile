CC=clang

#===DIRECTORIES===#
OBJ_DIR=src
OBJ_DIR32=src
SRC_DIR=src
TST_DIR=tests


LIBDEFLATE_INC=./src/extlib/libdeflate
LIBDEFLATE_LIB=./src/extlib/libdeflate/unix

INC=$(LIBDEFLATE_INC)
INC_PARAMS=$(foreach d, $(INC), -I$d)


#LIB=$(LIBDEFLATE_LIB)
LIB=.
LIB_PARAMS=$(foreach d, $(LIB), -I$d)

LIBS=-lm -lpthread

IDIR = src/headers
_DEPS = cleanup.h \
createDataObjects.h \
ezq.h \
mtezq.h\
fillDataObjects.h \
getDataObjects.h \
getSystemInfo.h \
init.h \
navigate.h \
numberHelper.h \
placeChunkedData.h \
placeData.h \
readMessage.h \
superblock.h \
utils.h \
getDataObjects.h \
../extlib/libdeflate/libdeflate.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = ../$(TST_DIR)/mexemulate.o \
cleanup.o \
createDataObjects.o \
ezq.o \
mtezq.o \
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
utils.o

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)



CFLAGS=-g -fsanitize=undefined --std=c99 -DNO_MEX $(INC_PARAMS) $(LIB_PARAMS)

all: mexemulate

mexemulate: $(OBJ) src/extlib/libdeflate/unix/libdeflate.a
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

src/extlib/libdeflate/unix/libdeflate.a:
	cd $(OBJ_DIR)/extlib/libdeflate/unix && $(MAKE)

rebuild: clean mexemulate

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o $(TST_DIR)/*.o *~ core $(INCDIR)/*~
