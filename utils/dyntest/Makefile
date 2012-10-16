BOOST_DIR = /fs/maxfli/lam/opt/boost_1_50_0

mutator: mutator.cpp
	g++ -o mutator -I$(BOOST_DIR) -I$(DYNINST_ROOT)/$(PLATFORM)/include -L$(DYNINST_ROOT)/$(PLATFORM)/lib -ldyninstAPI mutator.cpp

