#include <stdio.h>

long _INST_DISABLED = 0;
long _INST_INACTIVE = 1;
long _INST_ACTIVE   = 2;
long _INST_status   = _INST_DISABLED;

void _INST_print_flags(const char *tag, unsigned flags) {
    //bool CF = ((flags & 0x001) != 0);
    //bool PF = ((flags & 0x004) != 0);
    //bool AF = ((flags & 0x010) != 0);
    //bool ZF = ((flags & 0x040) != 0);
    //bool SF = ((flags & 0x080) != 0);
    //bool OF = ((flags & 0x800) != 0);
    bool CF = ((flags & 0x0100) != 0);
    bool PF = ((flags & 0x0400) != 0);
    bool AF = ((flags & 0x1000) != 0);
    bool ZF = ((flags & 0x4000) != 0);
    bool SF = ((flags & 0x8000) != 0);
    bool OF = ((flags & 0x0001) != 0);
    printf("%s flags: 0x%x OF=%c SF=%c ZF=%c AF=%c PF=%c CF=%c\n\n", 
            tag, flags,
            OF ? '1' : '0', SF ? '1' : '0',
            ZF ? '1' : '0', AF ? '1' : '0',
            PF ? '1' : '0', CF ? '1' : '0');
}

