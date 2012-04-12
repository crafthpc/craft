#CC = icc -w1
CC = g++
MPICC = mpicc
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

# this mess enables me to use Dyninst internals;
# hopefully I'll never have to use it for real
#DYNINST_CFLAGS += -I$(DYNINST_ROOT)/dyninst/dynutil/h \
				  #-I$(DYNINST_ROOT)/dyninst/symtabAPI/h \
				  #-I$(DYNINST_ROOT)/dyninst/instructionAPI/h \
				  #-I$(DYNINST_ROOT)/dyninst/parseAPI/h \
				  #-I$(DYNINST_ROOT)/dyninst/dataflowAPI/h \
				  #-I$(DYNINST_ROOT)/dyninst/patchAPI/h \
				  #-I$(DYNINST_ROOT)/dyninst/external \
				  #-Darch_x86_64 -Darch_64bit -Dos_linux  -Dcap_ptrace -Dcap_stripped_binaries \
				  #-Dcap_async_events -Dcap_threads -Dcap_dynamic_heap -Dcap_relocation -Dcap_dwarf \
				  #-Dcap_32_64 -Dcap_liveness -Dcap_fixpoint_gen -Dcap_noaddr_gen \
				  #-Dcap_mutatee_traps -Dcap_binary_rewriter -Dcap_registers -Dcap_instruction_api \
				  #-Dcap_serialization -Dcap_instruction_replacement -Dcap_tramp_liveness \
				  #-Dcap_gnu_demangler -Ddwarf_has_setframe -Dbug_syscall_changepc_rewind \
				  #-Dx86_64_unknown_linux2_4 \
				  #-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS  -D_REENTRANT=1 \
				  #-DUSES_DWARF_DEBUG -DBPATCH_LIBRARY  -DBPATCH_SET_MUTATIONS_ACTIVE \
				  #-DBPATCH_LIBRARY_F  -DNEW_TIME_TYPES -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 \
				  #-DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 \
				  #-DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 \
				  #-DHAVE_LIBELF=1 -DHAVE_LIBDWARF=1 -Dcap_have_libxml=1 -DHAVE_LIBXML2=1

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


TARGETS = $(PLATFORM)/libfpanalysis.so $(PLATFORM)/libfpshift.so $(PLATFORM)/fpconf $(PLATFORM)/fpinst
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

$(PLATFORM)/libfpshift.so: src/libfpshift.c
	$(MPICC) -fPIC -DPIC -shared -o $(PLATFORM)/libfpshift.so src/libfpshift.c

src/libfpshift.c: src/libfpshift.w
	extern/wrap/wrap.py -f -i pmpi_init_ -o src/libfpshift.c src/libfpshift.w

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

