all: rfunc calllibm

reproduce: rfunc calllibm
	./rfunc calllibm sin cos

rfunc: rfunc.cpp
	g++ -g -o rfunc -I$(DYNINST_ROOT)/include -L$(DYNINST_ROOT)/$(PLATFORM)/lib -ldyninstAPI rfunc.cpp

calllibm: calllibm.cpp
	g++ -g -o calllibm -lm calllibm.cpp

clean:
	rm -f rfunc calllibm

