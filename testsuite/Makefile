
#CC=icc
#CFLAGS=-g -O0 -Wall

CC=gcc
CXX=g++

#CFLAGS=-O0 -Werror
#CFLAGS+=-g -Wall -W -msse2 -mfpmath=sse -mno-80387
CFLAGS+=-g -msse2 -mfpmath=sse -mno-80387

#TESTS=blobtest pushpop regs
TESTS=arith calllib compare convert gauss intcopy lulesh mgbarsky mixed modes packed powloop sanity

all: $(TESTS) mixed.cfg

arith: util.o arith.c
	$(CC) $(CFLAGS) -o arith util.o arith.c

blobtest: blobtest.c
	$(CC) $(CFLAGS) -o blobtest blobtest.c

calllib: util.o calllib.c
	$(CC) $(CFLAGS) -Wno-implicit-function-declaration -o calllib util.o calllib.c -lm

compare: util.o compare.c
	$(CC) $(CFLAGS) -Wno-div-by-zero -o compare util.o compare.c

convert: util.o convert.c
	$(CC) $(CFLAGS) -o convert util.o convert.c -lm

gauss: util.o gauss.c
	$(CC) $(CFLAGS) -o gauss util.o gauss.c

intcopy: util.o intcopy.c
	$(CC) $(CFLAGS) -o intcopy util.o intcopy.c

lulesh: util.o lulesh.cc
	$(CXX) $(CFLAGS) -o lulesh util.o lulesh.cc

mgbarsky: mgbarsky.c
	$(CC) $(CFLAGS) -Wno-unused-variable -o mgbarsky mgbarsky.c -lm

mixed.cfg: mixed
	fpconf mixed >mixed.cfg

mixed: util.o mixed.c
	$(CC) $(CFLAGS) -o mixed util.o mixed.c

modes: util.o modes.c
	$(CC) $(CFLAGS) -o modes util.o modes.c

packed: util.o packed.c
	$(CC) $(CFLAGS) -o packed util.o packed.c -lm

powloop: util.o powloop.c
	$(CC) $(CFLAGS) -o powloop util.o powloop.c -lm

pushpop: util.o pushpop.c
	$(CC) $(CFLAGS) -o pushpop util.o pushpop.c

sanity: sanity.c
	$(CC) $(CFLAGS) -o sanity sanity.c

regs: util.o regs.c
	$(CC) $(CFLAGS) -o regs util.o regs.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c -o util.o util.c

.PHONY: all clean

clean:
	@rm -f $(TESTS) util.o

