# Makefile for fpinfo
#
# Mike Lam, UMD
#


DYNINST_PREFIX=$(DYNINST_ROOT)/$(PLATFORM)
DYNINST_LIBS=-ldyninstAPI -linstructionAPI -lpatchAPI
BOOST_INCLUDE=-I/fs/maxfli/lam/opt/boost_1_50_0
XED_KIT=/fs/dyninst/lam/opt/pin-2.12-58423-gcc.4.4.7-linux/extras/xed2-intel64

CFLAGS=-g -Wall $(BOOST_INCLUDE) -I$(XED_KIT)/include -I$(DYNINST_PREFIX)/include
LDFLAGS=$(XED_KIT)/lib/libxed.a -L$(DYNINST_PREFIX)/lib $(DYNINST_LIBS)

TARGETS=fpinfo

all: $(TARGETS)

fpinfo: fpinfo.cpp
	g++ -o $@ $(CFLAGS) $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean

