# Makefile for CRAFT
#
# Mike Lam, UMD
#


# pointer to the "extras/xed2-intel64" folder (for lib/libxed.a)
XED_KIT=

# modify these lines if you need the compiler/linker to find things in
# non-standard locations (e.g., Boost or libdwarf)
LOCAL_INC_DIRS =
LOCAL_LIB_DIRS =


# general compiler options
CC = g++
MPICC = mpicc
ifeq ($(PLATFORM),x86_64-unknown-linux2.4)
    CC += -D__X86_64__ -Darch_x86_64 -Dos_linux
endif
DEBUG_FLAGS = -g -DINCLUDE_DEBUG=1
WARN_FLAGS = -Wall -W -Wcast-align -Wno-deprecated-declarations

# Dyninst flags
DYNINST_CFLAGS  = -I$(DYNINST_ROOT)/$(PLATFORM)/include -I$(DYNINST_ROOT)/dyninst $(LOCAL_INC_DIRS)
DYNINST_LDFLAGS = -L$(DYNINST_ROOT)/$(PLATFORM)/lib $(LOCAL_LIB_DIRS) \
                  -ldyninstAPI -lstackwalk -lpcontrol -lpatchAPI -lparseAPI -linstructionAPI \
				  -lsymtabAPI -lsymLite -ldynDwarf -ldynElf -lcommon -pthread -ldl

# various compiler/linker flags
COMMON_CFLAGS  = -I./h -Iextern -I$(XED_KIT)/include -I $(LOCAL_INC_DIRS)
COMMON_LDFLAGS = -lrt $(XED_KIT)/lib/libxed.a -lelf $(LOCAL_LIB_DIRS)
LIB_CFLAGS     = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_CFLAGS) -msse2 -mfpmath=sse -O1
LIB_LDFLAGS    = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_LDFLAGS) -L./$(PLATFORM) $(COMMON_LDFLAGS)
CONF_CFLAGS    = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_CFLAGS) -msse2
CONF_LDFLAGS   = $(DEBUG_FLAGS) $(WARN_FLAGS) -L./$(PLATFORM) -lfpanalysis $(DYNINST_LDFLAGS) $(COMMON_LDFLAGS)
PROF_CFLAGS    = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_CFLAGS) -msse2
PROF_LDFLAGS   = $(DEBUG_FLAGS) $(WARN_FLAGS) -L./$(PLATFORM) -lfpanalysis $(DYNINST_LDFLAGS) $(COMMON_LDFLAGS)
DEPEND_CFLAGS  = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS) $(COMMON_CFLAGS) -msse2 -mfpmath=sse -O1

# modules to build for analysis library
LIB_MODULES = libfpanalysis fpflag FPAnalysis \
			  FPAnalysisCInst FPAnalysisTRange \
			  FPAnalysisDCancel FPAnalysisDNan \
			  FPAnalysisInplace FPAnalysisPointer \
			  FPSVPolicy FPSV FPSVSingle FPSVDouble \
			  FPSVConfigPolicy FPSVMemPolicy \
			  FPConfig FPShadowEntry FPReplaceEntry \
              FPBinaryBlob FPCodeGen FPContext FPLog \
			  FPDecoderXED FPDecoderIAPI FPFilterFunc \
			  FPOperand FPOperation FPSemantics

# executable modules
CONF_MODULES = fpconf
PROF_MODULES = fpinst

# make rules
TARGETS = $(PLATFORM)/libfpanalysis.so $(PLATFORM)/libfpshift.so $(PLATFORM)/fpconf $(PLATFORM)/fpinst
CONF_MODULE_FILES = $(foreach module, $(CONF_MODULES), $(PLATFORM)/$(module).o)
PROF_MODULE_FILES = $(foreach module, $(PROF_MODULES), $(PLATFORM)/$(module).o)
LIB_MODULE_FILES = $(foreach module, $(LIB_MODULES), $(PLATFORM)/$(module).o)
DEPEND_MODULES = $(CONF_MODULES) $(PROF_MODULES) $(LIB_MODULES)
DEPEND_FILES = $(foreach file, $(DEPEND_MODULES), src/$(file).depends)
EXTERN_LIBS = 


# general targets

all: $(TARGETS) fpviewer
	@echo -e "Build of FPAnalysis complete."

fpviewer: viewer/*.java
	cd viewer && make

tags: 
	ctags --no-members src/*.cpp
	ctags --no-members -a h/*.h
	ctags --no-members -a viewer/*.java


# specific targets

$(PLATFORM)/:
	mkdir -p $(PLATFORM)

$(PLATFORM)/libfpanalysis.so: $(PLATFORM)/ $(EXTERN_LIBS) $(LIB_MODULE_FILES)
	$(CC) $(LIB_MODULE_FILES) $(LIB_LDFLAGS) -shared -o $@ 

$(PLATFORM)/libfpshift.so: src/libfpshift.c
	$(MPICC) -fPIC -DPIC -shared -o $(PLATFORM)/libfpshift.so src/libfpshift.c

src/libfpshift.c: src/libfpshift.w
	extern/wrap/wrap.py -f -i pmpi_init_ -o src/libfpshift.c src/libfpshift.w

$(PLATFORM)/fpconf: $(CONF_MODULE_FILES) $(PLATFORM)/libfpanalysis.so
	$(CC) $(CONF_MODULE_FILES) $(CONF_LDFLAGS) -o $@

$(PLATFORM)/fpinst: $(PROF_MODULE_FILES) $(PLATFORM)/libfpanalysis.so
	$(CC) $(PROF_MODULE_FILES) $(PROF_LDFLAGS) -o $@

$(LIB_MODULE_FILES): $(PLATFORM)/%.o: src/%.cpp
	$(CC) $(LIB_CFLAGS) -fPIC -c -o $@ $<

$(CONF_MODULE_FILES): $(PLATFORM)/%.o: src/%.cpp
	$(CC) $(CONF_CFLAGS) -c -o $@ $<

$(PROF_MODULE_FILES): $(PLATFORM)/%.o: src/%.cpp
	$(CC) $(PROF_CFLAGS) -c -o $@ $<

$(DEPEND_FILES): src/%.depends: src/%.cpp
	$(CC) -MM -MF $@ $(DEPEND_CFLAGS) $<


# debug (output preprocessing) targets

#$(PLATFORM)/libfpanalysis.o: src/libfpanalysis.cpp h/fprintf.cpp
	#cpp $(LIB_CFLAGS) -o processed_libfpanalysis.cpp $<
	#$(CC) $(LIB_CFLAGS) -fPIC -c -o $@ $<

#$(PLATFORM)/FPDecoderXED.o: src/FPDecoderXED.cpp
	#cpp $(LIB_CFLAGS) -o processed_decoderXED.cpp $<
	#$(CC) $(LIB_CFLAGS) -fPIC -c -o $@ $<


# misc targets

clean:
	rm -f $(CONF_MODULE_FILES) $(PROF_MODULE_FILES) $(LIB_MODULE_FILES) $(TARGETS)

cleandepend:
	rm -f $(DEPEND_FILES)

.PHONY: clean cleandepend all fpviewer tags

-include $(DEPEND_FILES)

