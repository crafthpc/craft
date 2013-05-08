#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    unsigned long *_INST_main_inst_count = NULL;
}

// currently MUST be known a priori
unsigned long INST_COUNT_SIZE = 2;

void _INST_init_analysis() {
    
    // read config settings
    // (currently just the instruction count)
    char buffer[1024];
    char *key, *value;
    FILE *fc = fopen("mutant.cfg", "r");
    if (fc != NULL) {
        fgets(buffer, 1024, fc);
        key = strtok(buffer, "=");
        if (key != NULL) {
            value = strtok(NULL, "=");
            if (value != NULL) {
                if (strcmp(key,"num_instructions")==0) {
                    INST_COUNT_SIZE = strtoul(value, NULL, 10);
                }
            }
        }
        fclose(fc);
    }

    // allocate and initialize instruction count array
    _INST_main_inst_count = (unsigned long*)malloc(sizeof(unsigned long)*INST_COUNT_SIZE);
    if (!_INST_main_inst_count) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    memset(_INST_main_inst_count, 0, sizeof(unsigned long)*INST_COUNT_SIZE);
}

void _INST_cleanup_analysis() {

    // print instruction counts and free memory
    for (unsigned i=0; i<INST_COUNT_SIZE; i++) {
        fprintf(stderr, "INSN #%05u: %lu\n", i, _INST_main_inst_count[i]);
    }
    if (_INST_main_inst_count) {
        free(_INST_main_inst_count);
    }
}

