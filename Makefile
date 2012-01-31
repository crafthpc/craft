#CC = icc -w1
CC = g++
MACHINE = $(shell hostname)
UTILDIR = /fs/dyninst/utils
ULIBDIR = $(UTILDIR)/machines/$(MACHINE)/64/lib64

ifeq ($(PLATFORM),x86_64-unknown-linux2.4)
    CC += -D__X86_64__ -Darch_x86_64 -Dos_linux
endif

DEBUG_FLAGS = -g -DINCLUDE_DEBUG=1
#DEBUG_FLAGS = -g
#WARN_FLAGS = -Wall -W -Wcast-align
WARN_FLAGS = -Wall -Werror -W -Wcast-align
#WARN_FLAGS = -Wall -Werror -W -falign-functions=16
#WARN_FLAGS = -Wall -Werror -W -mpreferred-stack-boundary=4
#WARN_FLAGS = -Wall -Werror -W -Wpointer-arith -Wwrite-strings -fno-common
COMMON_LIBS = -lrt -rdynamic -lxed -lgmp -lmpfr -lgc 
COMMON_INCLUDES = -I./h -Iextern -Iextern/xed/include -Iextern/mpfr/include -Iextern/gc/include

DYNINST_CFLAGS = -I$(DYNINST_ROOT)/include \
				 -I$(DYNINST_ROOT)/dyninst

DYNINST_LDFLAGS = -L$(DYNINST_ROOT)/$(PLATFORM)/lib -L$(ULIBDIR) \
                  -lcommon -ldyninstAPI -lsymtabAPI -linstructionAPI -lstackwalk

LIB_CFLAGS    = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_INCLUDES) -msse2 -mfpmath=sse -O1
LIB_LDFLAGS   = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_LDFLAGS) -L./$(PLATFORM) $(COMMON_LIBS)
CONF_CFLAGS   = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_INCLUDES) -msse2
CONF_LDFLAGS  = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_LDFLAGS) -L./$(PLATFORM) $(COMMON_LIBS) -lfpanalysis
PROF_CFLAGS   = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_INCLUDES) -msse2
PROF_LDFLAGS  = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_LDFLAGS) -L./$(PLATFORM) $(COMMON_LIBS) -lfpanalysis
DEPEND_CFLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_INCLUDES) -msse2 -mfpmath=sse -O1


LIB_MODULES = libfpanalysis fpflag FPAnalysis \
			  FPAnalysisCInst FPAnalysisTRange \
			  FPAnalysisDCancel FPAnalysisDNan \
			  FPAnalysisInplace FPAnalysisPointer \
			  FPSVPolicy FPSV FPSVSingle FPSVDouble \
			  FPConfig FPSVConfigPolicy FPShadowEntry FPReplaceEntry \
              FPBinaryBlob FPCodeGen FPContext FPLog \
			  FPDecoderXED FPDecoderIAPI FPFilterFunc \
			  FPOperand FPOperation FPSemantics

CONF_MODULES = fpconf

PROF_MODULES = fpinst


TARGETS = $(PLATFORM)/libfpanalysis.so $(PLATFORM)/fpconf $(PLATFORM)/fpinst
CONF_MODULE_FILES = $(foreach module, $(CONF_MODULES), $(PLATFORM)/$(module).o)
PROF_MODULE_FILES = $(foreach module, $(PROF_MODULES), $(PLATFORM)/$(module).o)
LIB_MODULE_FILES = $(foreach module, $(LIB_MODULES), $(PLATFORM)/$(module).o)
DEPEND_MODULES = $(CONF_MODULES) $(PROF_MODULES) $(LIB_MODULES)
DEPEND_FILES = $(foreach file, $(DEPEND_MODULES), src/$(file).depends)
#EXTERN_LIBS = $(PLATFORM)/libxed.so $(PLATFORM)/libmpfr.so $(PLATFORM)/libgc.so
EXTERN_LIBS = $(PLATFORM)/libxed.so $(PLATFORM)/libgc.so

all: $(TARGETS) fpviewer
	@echo -e "Build of FPAnalysis complete."

fpviewer: viewer/*.java
	cd viewer && make

$(PLATFORM)/:
	mkdir -p $(PLATFORM)

$(PLATFORM)/libfpanalysis.so: $(PLATFORM)/ $(EXTERN_LIBS) $(LIB_MODULE_FILES)
	$(CC) $(LIB_LDFLAGS) -shared -o $@ $(LIB_MODULE_FILES)

$(PLATFORM)/fpconf: $(CONF_MODULE_FILES) $(PLATFORM)/libfpanalysis.so
	$(CC) $(CONF_CFLAGS) $(CONF_LDFLAGS) -o $@ $(CONF_MODULE_FILES)

$(PLATFORM)/fpinst: $(PROF_MODULE_FILES) $(PLATFORM)/libfpanalysis.so
	$(CC) $(PROF_CFLAGS) $(PROF_LDFLAGS) -o $@ $(PROF_MODULE_FILES)

$(PLATFORM)/libxed.so:
	cp extern/xed/lib/$(PLATFORM)/libxed.so $(PLATFORM)/libxed.so

$(PLATFORM)/libmpfr.so:
	cp extern/mpfr/lib/$(PLATFORM)/libmpfr.so $(PLATFORM)/libmpfr.so
	cd $(PLATFORM) && ln -s libmpfr.so libmpfr.so.4

$(PLATFORM)/libgc.so:
	cp extern/gc/lib/$(PLATFORM)/libgc.so $(PLATFORM)/libgc.so
	cd $(PLATFORM) && ln -s libgc.so libgc.so.1

$(LIB_MODULE_FILES): $(PLATFORM)/%.o: src/%.cpp
	$(CC) $(LIB_CFLAGS) -fPIC -c -o $@ $<

$(CONF_MODULE_FILES): $(PLATFORM)/%.o: src/%.cpp
	$(CC) $(CONF_CFLAGS) -c -o $@ $<

$(PROF_MODULE_FILES): $(PLATFORM)/%.o: src/%.cpp
	$(CC) $(PROF_CFLAGS) -c -o $@ $<

$(DEPEND_FILES): src/%.depends: src/%.cpp
	$(CC) -MM -MF $@ $(DEPEND_CFLAGS) $<

$(PLATFORM)/libfpanalysis.o: src/libfpanalysis.cpp h/fprintf.cpp
	#cpp $(LIB_CFLAGS) -o processed_libfpanalysis.cpp $<
	$(CC) $(LIB_CFLAGS) -fPIC -c -o $@ $<

$(PLATFORM)/FPDecoderXED.o: src/FPDecoderXED.cpp
	#cpp $(LIB_CFLAGS) -o processed_decoderXED.cpp $<
	$(CC) $(LIB_CFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f $(CONF_MODULE_FILES) $(PROF_MODULE_FILES) $(LIB_MODULE_FILES) $(TARGETS)

cleandepend:
	rm -f $(DEPEND_FILES)

.PHONY: clean all fpviewer

-include $(DEPEND_FILES)

