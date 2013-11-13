DYNINST_PREFIX=$(DYNINST_ROOT)/$(PLATFORM)
DYNINST_LIBS=-ldyninstAPI -lstackwalk -lpcontrol -lpatchAPI -lparseAPI -linstructionAPI -lsymtabAPI -lsymLite -ldynDwarf -ldynElf -lcommon -pthread -ldl
BOOST_INCLUDE=-I/fs/maxfli/lam/opt/boost_1_50_0

CFLAGS=-g $(BOOST_INCLUDE) -I$(DYNINST_PREFIX)/include
LDFLAGS=-L$(DYNINST_PREFIX)/lib $(DYNINST_LIBS)

TARGETS=listmodules

all: $(TARGETS)

listmodules: listmodules.cpp
	g++ -o $@ $(CFLAGS) $(LDFLAGS) $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean

