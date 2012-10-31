#BOOST_INCLUDE=-I/fs/maxfli/lam/opt/boost_1_50_0
BOOST_INCLUDE=

LIBS=-ldyninstAPI -lstackwalk -lpcontrol -lpatchAPI -lparseAPI -linstructionAPI -lsymtabAPI -lsymLite -ldynDwarf -ldynElf -lcommon -pthread -ldl

mutator: mutator.cpp
	g++ -o mutator $(BOOST_INCLUDE) -I$(DYNINST_PREFIX)/include -L$(DYNINST_PREFIX)/lib mutator.cpp $(LIBS)

clean:
	rm -f mutator

