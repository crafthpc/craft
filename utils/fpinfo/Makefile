DYNINST_PREFIX=$(DYNINST_ROOT)/$(PLATFORM)
DYNINST_LIBS=-ldyninstAPI -linstructionAPI -lpatchAPI
BOOST_INCLUDE=-I/fs/maxfli/lam/opt/boost_1_50_0

CFLAGS=-g $(BOOST_INCLUDE) -I$(DYNINST_PREFIX)/include
LDFLAGS=-L$(DYNINST_PREFIX)/lib $(DYNINST_LIBS)

TARGETS=fpinfo

all: $(TARGETS)

fpinfo: fpinfo.cpp
	g++ -o $@ $(CFLAGS) $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean

