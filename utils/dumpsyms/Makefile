CC = g++
MACHINE = $(shell hostname)
UTILDIR = /fs/dyninst/utils
ULIBDIR = $(UTILDIR)/machines/$(MACHINE)/64/lib64

ifeq ($(PLATFORM),x86_64-unknown-linux2.4)
    CC += -D__X86_64__ -Darch_x86_64 -Dos_linux
endif

DEBUG_FLAGS = -g -DINCLUDE_DEBUG=1
WARN_FLAGS = -Wall -Werror -W -Wcast-align
DYNINST_CFLAGS = -I$(DYNINST_ROOT)/include
DYNINST_LDFLAGS = -L$(DYNINST_ROOT)/$(PLATFORM)/lib -L$(ULIBDIR) \
                  -ldyninstAPI -lsymtabAPI

DUMP_CFLAGS   = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_CFLAGS)
DUMP_LDFLAGS  = $(DEBUG_FLAGS) $(WARN_FLAGS) $(DYNINST_LDFLAGS)

all: dumpsyms

dumpsyms: dumpsyms.cpp
	$(CC) $(DUMP_CFLAGS) $(DUMP_LDFLAGS) -o $@ dumpsyms.cpp

clean:
	rm -f dumpsyms

.PHONY: clean all

