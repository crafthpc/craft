#include "libfpc.h"

int _INST_cflag = 0;

int _INST_fprintf(FILE* fd, const char *fmt, ...) {
    if (_INST_cflag) {
        va_list args;
        va_start(args, fmt);
        return vfprintf(fd, fmt, args);
    }
    _INST_cflag = 1;
    //puts("_INST_fprintf: "); puts(fmt); puts ("\n");
#define PRINT_ORIG fprintf
#define PRINT_VORIG vfprintf
#define PRINT_OUT fd
#include "fprintf.c"
#undef PRINT_ORIG
#undef PRINT_VORIG
#undef PRINT_OUT
    _INST_cflag = 0;
    return ret;
}

int _INST_printf(const char *fmt, ...) {
    if (_INST_cflag) {
        va_list args;
        va_start(args, fmt);
        return vprintf(fmt, args);
    }
    _INST_cflag = 1;
    //puts("_INST_printf: "); puts(fmt); puts ("\n");
#define PRINT_ORIG fprintf
#define PRINT_VORIG vfprintf
#define PRINT_OUT stdout
#include "fprintf.c"
#undef PRINT_ORIG
#undef PRINT_VORIG
#undef PRINT_OUT
    _INST_cflag = 0;
    return ret;
}

int _INST_sprintf(char *str, const char *fmt, ...) {
    if (_INST_cflag) {
        va_list args;
        va_start(args, fmt);
        return vsprintf(str, fmt, args);
    }
    _INST_cflag = 1;
    //puts("_INST_sprintf(\""); puts(fmt); puts("\")\n");
#define PRINT_ORIG sprintf
#define PRINT_VORIG vsprintf
#define PRINT_OUT str
#define PRINT_INC str
#include "fprintf.c"
#undef PRINT_ORIG
#undef PRINT_VORIG
#undef PRINT_OUT
#undef PRINT_INC
    (*str++) = '\0';
    _INST_cflag = 0;
    return ret;
}


/*
 * FORTRAN STUB FOR THIS ROUTINE:
 *
 * subroutine inst_fortran_report ( val, lbl )
 * implicit none
 * double precision val
 * character*(*) lbl
 * write(*,*) lbl, val
 * end
 */

extern "C"
void _INST_fortran_report(const double *val, const char *lbl, const int lbl_len) {
    /*
     *static char buffer[1024];
     *_INST_status = _INST_ACTIVE;
     *void* a_ptr = *(void**)(val);
     *double a_dbl = *(double*)(val);
     *strncpy(buffer, lbl, lbl_len);
     *buffer[lbl_len] = '\0';
     *if (_INST_Main_PointerAnalysis && _INST_Main_PointerAnalysis->isSVPtr(a_ptr)) {
     *    printf("REPORT %s: %.8g [%p] => ^%.8g\n", buffer, *val, a_ptr,
     *            ((FPSV*)a_ptr)->getValue(IEEE_Double).data.dbl);
     *} else if (_INST_Main_InplaceAnalysis && _INST_Main_InplaceAnalysis->isReplaced(a_dbl)) {
     *    printf("REPORT %s: %.8g [%p] => ^%.8g\n", buffer, *val, a_ptr,
     *            _INST_Main_InplaceAnalysis->extractFloat(a_dbl));
     *} else {
     *    printf("REPORT %s: %.8g\n", buffer, *val);
     *}
     *_INST_status = _INST_INACTIVE;
     */
}
