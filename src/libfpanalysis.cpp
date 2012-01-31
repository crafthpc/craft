/*
 * libfpanalysis.cpp
 *
 * Floating-point analysis library
 *
 * Originally written by Mike Lam, Fall 2008
 */

//#include "fpanalysis.h"
#include "fpflag.h"

#include <stdarg.h>
#include <unistd.h>

#include "FPContext.h"
#include "FPConfig.h"
#include "FPLog.h"
#include "FPDecoderXED.h"
#include "FPDecoderIAPI.h"
#include "FPAnalysisCInst.h"
#include "FPAnalysisDCancel.h"
#include "FPAnalysisDNan.h"
#include "FPAnalysisTRange.h"
#include "FPAnalysisPointer.h"
#include "FPAnalysisInplace.h"

using namespace FPInst;

FPContext *mainContext;
FPConfig  *mainConfig;
FPLog     *mainLog;
FPDecoder *mainDecoder;

FPAnalysis               *mainNullAnalysis;
extern FPAnalysisCInst   *_INST_Main_CInstAnalysis;
const long cinstID   =  1;
extern FPAnalysisDCancel *_INST_Main_DCancelAnalysis;
const long dcancelID = 11;
extern FPAnalysisDNan *_INST_Main_DNanAnalysis;
const long dnanID    = 12;
extern FPAnalysisTRange  *_INST_Main_TRangeAnalysis;
const long trangeID  = 21;
extern FPAnalysisPointer *_INST_Main_PointerAnalysis;
const long svptrID   = 31;
extern FPAnalysisInplace *_INST_Main_InplaceAnalysis;
const long svinpID   = 32;
FPAnalysis               *allAnalyses[10];

unsigned long *_INST_reg_eip;
unsigned long *_INST_reg_eflags;
unsigned long *_INST_reg_eax;
unsigned long *_INST_reg_ebx;
unsigned long *_INST_reg_ecx;
unsigned long *_INST_reg_edx;
unsigned long *_INST_reg_ebp;
unsigned long *_INST_reg_esp;
unsigned long *_INST_reg_esi;
unsigned long *_INST_reg_edi;
unsigned long *_INST_reg_r8;
unsigned long *_INST_reg_r9;
unsigned long *_INST_reg_r10;
unsigned long *_INST_reg_r11;
unsigned long *_INST_reg_r12;
unsigned long *_INST_reg_r13;
unsigned long *_INST_reg_r14;
unsigned long *_INST_reg_r15;

//static const char *DEFAULT_LOG_FILE = "analysis.log";

const size_t LOG_FILENAME_LEN = 2048;
char _INST_log_file[LOG_FILENAME_LEN] = "output.log";

size_t analysisCount;
size_t _INST_count;
size_t _INST_fast_count;

// {{{ PROFILER STUFF
#include <execinfo.h>
#include <signal.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/time.h> 
#include <sys/types.h> 
#include <time.h> 
#include <ucontext.h>
#include <unistd.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif
void _INST_alarm_handler(int /*sig*/, siginfo_t* /*info*/, void* /*context*/)
{
    /*
     *if (_INST_Main_PointerAnalysis != NULL) {
     *    printf("%10ld total FPSV allocation(s)  %10ld GC FPSV allocation(s)  %10ld kbytes  %10ld value writebacks  %10ld ptr writebacks\n", 
     *            _INST_Main_PointerAnalysis->getNumAllocations(),
     *            _INST_Main_PointerAnalysis->getNumGCAllocations(),
     *            (_INST_Main_PointerAnalysis->getNumAllocations() + _INST_Main_PointerAnalysis->getNumGCAllocations())*sizeof(FPSVDouble)/1024,
     *            _INST_Main_PointerAnalysis->getNumValueWriteBacks(),
     *            _INST_Main_PointerAnalysis->getNumPtrWriteBacks());
     *    //_INST_Main_PointerAnalysis->printLivePtrTableHealth();
     *}
     */

    /*
    ucontext_t *ucontext = (ucontext_t*)context;
    void *rip = (void *)ucontext->uc_mcontext.gregs[REG_RIP];
    printf("SIGPROF RIP=%p\n", rip);
    */

    /*
    void *trace[10];
    int i, j, trace_size;
    char **messages = (char **)NULL;
    trace_size = backtrace(trace, 10);
    trace[1] = rip;
    messages = backtrace_symbols(trace, trace_size);
    for (i=1; i<trace_size; i++) {
        for (j=0; j<i-1; j++) {
            printf("  ");
        }
        printf("%s\n", messages[i]);
    }
    */

}
// }}}

void _INST_null() {
    return;
}

void _INST_sigsegv_handler(int)
{
    printf("handling SIGSEGV\n");
    if (mainLog) {
        printf("%s\n", mainLog->getStackTrace().c_str());
    }
    exit(-1);
}

void _INST_set_config (char* setting)
{
    _INST_status = _INST_ACTIVE;
    FPConfig::getMainConfig()->addSetting(setting);
    _INST_status = _INST_INACTIVE;
}

void _INST_begin_profiling ()
{
    struct sigaction sa;
    sa.sa_sigaction = _INST_alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGPROF, &sa, 0);
    struct itimerval t;
    t.it_value.tv_sec = 0;
    t.it_interval.tv_sec = 0;
    //t.it_value.tv_usec = 10000;
    //t.it_interval.tv_usec = 10000;
    t.it_value.tv_usec    = 500000;  // buzz
    t.it_interval.tv_usec = 500000;  // buzz
    //t.it_value.tv_usec = 1000000;
    //t.it_interval.tv_usec = 1000000;
    setitimer(ITIMER_PROF, &t, 0);
    printf("FPAnalysis: Registered SIGPROF handler\n");
}

string sanitize_filename(const char *input)
{
    string fn = "";
    size_t i;
    char c;
    for (i=0; i<strlen(input); i++) {
        c = input[i];
        if (isalnum(c) || c == '_' || c == '-') {
            fn += c;
        }
    }
    return fn;
}

void _INST_init_analysis ()
{
    stringstream status;
    status.clear();
    status.str("");

    // default context object
    mainContext = new FPContext();
    mainContext->saveAllFPR();
    _INST_status = _INST_ACTIVE;
    status << "Initializing analysis using default context." << endl;

    cout << "FPAnalysis: Initializing..." << endl;

    // set up register pointers
    _INST_reg_eip = &mainContext->reg_eip;
    _INST_reg_eflags = &mainContext->reg_eflags;
    _INST_reg_eax = &mainContext->reg_eax;
    _INST_reg_ebx = &mainContext->reg_ebx;
    _INST_reg_ecx = &mainContext->reg_ecx;
    _INST_reg_edx = &mainContext->reg_edx;
    _INST_reg_ebp = &mainContext->reg_ebp;
    _INST_reg_esp = &mainContext->reg_esp;
    _INST_reg_esi = &mainContext->reg_esi;
    _INST_reg_edi = &mainContext->reg_edi;
    _INST_reg_r8  = &mainContext->reg_r8;
    _INST_reg_r9  = &mainContext->reg_r9;
    _INST_reg_r10 = &mainContext->reg_r10;
    _INST_reg_r11 = &mainContext->reg_r11;
    _INST_reg_r12 = &mainContext->reg_r12;
    _INST_reg_r13 = &mainContext->reg_r13;
    _INST_reg_r14 = &mainContext->reg_r14;
    _INST_reg_r15 = &mainContext->reg_r15;

    // main configuration file
    //mainConfig = new FPConfig("fpinst.cfg");
    mainConfig = FPConfig::getMainConfig();
    status << "Configuration:" << endl << mainConfig->getSummary();
    
    // main log file
    const char *logfile = mainConfig->getValueC("log_file");
    const char *appname = mainConfig->getValueC("app_name");
    const char *tag = getenv("TAG");
    if (!tag) {
        tag = mainConfig->getValueC("tag");
    }
    if (logfile) {
        strncpy(_INST_log_file, logfile, LOG_FILENAME_LEN);
    } else {
        stringstream log;
        log.clear();
        log.str("");
        if (appname) {
            log << sanitize_filename(appname) << "-";
        }
        if (tag) {
            log << sanitize_filename(tag) << "-";
        }
        char hostname[256] = "";
        if (gethostname(hostname, 256) == 0) {
            log << sanitize_filename(hostname) << "-";
        }
        log << getpid();
        log << ".log";
        strncpy(_INST_log_file, log.str().c_str(), LOG_FILENAME_LEN);
    }
    if (appname) {
        mainLog = new FPLog(_INST_log_file, appname);
    } else {
        mainLog = new FPLog(_INST_log_file);
    }
    status << "Opened log file: " << _INST_log_file << endl;

    // debug info
    const char *DEBUG_FILE = mainConfig->getValueC("debug_file");
    if (DEBUG_FILE) {
        status << "Using debug info from file: " << DEBUG_FILE << endl;
        mainLog->setDebugFile(string(DEBUG_FILE));
    } else {
        status << "No debug info available.";
    }
    if (mainConfig->getValue("enable_stwalk") == "yes") {
        mainLog->enableStackWalks();
        status << "Stackwalks enabled." << endl;
    }

    // the XED decoder is currently more accurate
    mainDecoder = new FPDecoderXED();
    //mainDecoder = new FPDecoderIAPI();
    status << "XED decoder initialized." << endl;
    
    // {{{ initialize various analyses
    analysisCount = 0;
    if (mainConfig->hasValue("c_inst") && mainConfig->getValue("c_inst") == "yes") {
        FPAnalysisCInst::getInstance()->configure(mainConfig, mainDecoder, mainLog, mainContext);
        allAnalyses[analysisCount++] = FPAnalysisCInst::getInstance();
        status << "c_inst: initialized" << endl;
    }
    if (mainConfig->hasValue("d_cancel") && mainConfig->getValue("d_cancel") == "yes") {
        FPAnalysisDCancel::getInstance()->configure(mainConfig, mainDecoder, mainLog, mainContext);
        allAnalyses[analysisCount++] = FPAnalysisDCancel::getInstance();
        status << "d_cancel: initialized" << endl;
    }
    if (mainConfig->hasValue("d_nan") && mainConfig->getValue("d_nan") == "yes") {
        FPAnalysisDNan::getInstance()->configure(mainConfig, mainDecoder, mainLog, mainContext);
        allAnalyses[analysisCount++] = FPAnalysisDNan::getInstance();
        status << "d_nan: initialized" << endl;
    }
    if (mainConfig->hasValue("t_range") && mainConfig->getValue("t_range") == "yes") {
        FPAnalysisTRange::getInstance()->configure(mainConfig, mainDecoder, mainLog, mainContext);
        allAnalyses[analysisCount++] = FPAnalysisTRange::getInstance();
        status << "t_range: initialized" << endl;
    }
    if (mainConfig->hasValue("sv_ptr") && mainConfig->getValue("sv_ptr") == "yes") {
        FPAnalysisPointer::getInstance()->configure(mainConfig, mainDecoder, mainLog, mainContext);
        allAnalyses[analysisCount++] = FPAnalysisPointer::getInstance();
        status << "sv_ptr: initialized" << endl;
    }
    if (mainConfig->hasValue("sv_inp") && mainConfig->getValue("sv_inp") == "yes") {
        FPAnalysisInplace::getInstance()->configure(mainConfig, mainDecoder, mainLog, mainContext);
        allAnalyses[analysisCount++] = FPAnalysisInplace::getInstance();
        status << "sv_inp: initialized" << endl;
    }
    if (analysisCount == 0) {
        mainNullAnalysis = new FPAnalysis();
        allAnalyses[analysisCount++] = mainNullAnalysis;
        status << "null instrumentation initialized" << endl;
    }
    // }}}

    _INST_count = 0;
    _INST_fast_count = 0;

    if (mainConfig->getValue("enable_profiling") == "yes") {
        _INST_begin_profiling();
    }

    mainLog->addMessage(STATUS, 0, "Profiler initialized.", status.str(), "");
    cout << "FPAnalysis: Profiler initialized w/ configuration:" << endl;
    cout << mainConfig->getSummary().c_str();

    _INST_status = _INST_DISABLED;
    mainContext->restoreAllFPR();
}

void _INST_enable_analysis ()
{
    _INST_status = _INST_ACTIVE;
    printf("FPAnalysis: Profiler enabled.\n");
    _INST_status = _INST_INACTIVE;
}

void _INST_register_inst (long iidx, void* addr, char* bytes, long nbytes)
{
    //printf("registering instruction #%ld:  %p @ %p  (%ld bytes)  ",
            //iidx, addr, bytes, nbytes);
    //FPDecoderXED::printInstBytes(stdout, (unsigned char*)bytes, nbytes);
    _INST_status = _INST_ACTIVE;
    FPSemantics *inst = mainDecoder->decode(iidx, addr, (unsigned char*)bytes, nbytes);
    //printf("  %s\n", inst->getDisassembly().c_str());
    size_t i;
    for (i=0; i<analysisCount; i++) {
        allAnalyses[i]->registerInstruction(inst);
    }
    _INST_status = _INST_INACTIVE;
}

        //printf(#rt " " #of " => " #nf "(" #at " %8.2e)\n", (x));

        //__asm__ ("fxsave %0;" : : "m" (temp_fxsave_state));
        //__asm__ ("emms;");
        //__asm__ ("fxrstor %0;" : : "m" (temp_fxsave_state));
        
struct fxsave_data temp_fxsave_state;

#define CHECK_FPSTSW \
    unsigned fpstsw = 0; \
    __asm__ ("fnstsw %0;" : : "m" (fpstsw)); \
    if (fpstsw & 0x40) { \
        printf("FP STACK OVERFLOW\n"); \
    } \
    if (mainContext->fxsave_state->fsw & 0x40) { \
        printf("FP STACK OVERFLOW IN FPCONTEXT\n"); \
    }

        //mainContext->fxsave_state->fsw = 0; 
        //mainContext->fxsave_state->ftw = 0; 
       

#define PASS_THROUGH_PTRFUNC_ONEARG(of,op,rt,at) \
    extern "C" rt _ORIG##of(at) __attribute__((weak)); \
    rt of(at x) \
    { \
        if (_INST_status != _INST_INACTIVE) { \
            return _ORIG##of(x); \
        } \
        rt ret = (rt)0.0; \
        CHECK_FPSTSW \
        _INST_status = _INST_ACTIVE; \
        if (_INST_Main_PointerAnalysis) { \
            ret = (rt)_INST_Main_PointerAnalysis->handleUnaryReplacedFunc(op, x); \
        } \
        if (_INST_Main_InplaceAnalysis) { \
            ret = (rt)_INST_Main_InplaceAnalysis->handleUnaryReplacedFunc(op, x); \
        } \
        _INST_status = _INST_INACTIVE; \
        return ret; \
    }

            //if (_INST_status == _INST_ACTIVE) printf(" calling _ORIG" #of ); 
        //printf("PASSTHROUGH: " #of " %lx (%g)", 
                //*(uint64_t*)(&x), *(float*)(&x)); 
        //printf(" = %lx (%g)\n",
                //*(uint64_t*)(&ret), *(float*)(&ret));

        //printf("PASSTHROUGH: " #of " %lx (%g) = %lx (%g)\n",
                //*(uint64_t*)(&x), *(float*)(&x),
                //*(uint64_t*)(&ret), *(float*)(&ret));
        //__asm__ ("emms;");

#define PASS_THROUGH_PTRFUNC_TWOARGS(of,op,rt,a1t,a2t) \
    extern "C" rt _ORIG##of(a1t, a2t) __attribute__((weak)); \
    rt of(a1t x, a2t y) \
    { \
        if (_INST_status != _INST_INACTIVE) { \
            return _ORIG##of(x, y); \
        } \
        rt ret = (rt)0.0; \
        CHECK_FPSTSW \
        _INST_status = _INST_ACTIVE; \
        if (_INST_Main_PointerAnalysis) { \
            ret = (rt)_INST_Main_PointerAnalysis->handleBinaryReplacedFunc(op, x, y); \
        } \
        if (_INST_Main_InplaceAnalysis) { \
            ret = (rt)_INST_Main_InplaceAnalysis->handleBinaryReplacedFunc(op, x, y); \
        } \
        _INST_status = _INST_INACTIVE; \
        return ret; \
    }

        //printf("PASSTHROUGH: %lx (%g) " #of " %lx (%g) = %lx (%g)\n",
                //*(uint64_t*)(&x),   *(float*)(&x),
                //*(uint64_t*)(&y),   *(float*)(&y),
                //*(uint64_t*)(&ret), *(float*)(&ret));



#define PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(of,nf,rt,a1t,a2t) \
    extern "C" rt _ORIG##of(a1t, a2t) __attribute__((weak)); \
    rt of(a1t x, a2t y) \
    { \
        if (_INST_status != _INST_INACTIVE) { \
            return _ORIG##of(x, y); \
        } \
        rt ret = (rt)0.0; \
        CHECK_FPSTSW \
        _INST_status = _INST_ACTIVE; \
        if (_INST_Main_PointerAnalysis) { \
            ret = (rt)_INST_Main_PointerAnalysis->handleReplacedFunc##nf(x, y); \
        } \
        if (_INST_Main_InplaceAnalysis) { \
            ret = (rt)_INST_Main_InplaceAnalysis->handleReplacedFunc##nf(x, y); \
        } \
        _INST_status = _INST_INACTIVE; \
        return ret; \
    }

        //printf("PASSTHROUGH: " #of "\n");

// {{{ double-precision pass-throughs

PASS_THROUGH_PTRFUNC_ONEARG(_INST_fabs,    OP_ABS,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_ceil,    OP_CEIL,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_erf,     OP_ERF,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_erfc,    OP_ERFC,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_exp,     OP_EXP,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_exp2,    OP_EXP2,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_floor,   OP_FLOOR, double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_log,     OP_LOG,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_logb,    OP_LOGB,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_log10,   OP_LOG10, double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_sqrt,    OP_SQRT,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cbrt,    OP_CBRT,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_trunc,   OP_TRUNC, double, double);

PASS_THROUGH_PTRFUNC_ONEARG(_INST_sin,     OP_SIN,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cos,     OP_COS,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_tan,     OP_TAN,   double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_asin,    OP_ASIN,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_acos,    OP_ACOS,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_atan,    OP_ATAN,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_sinh,    OP_SINH,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cosh,    OP_COSH,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_tanh,    OP_TANH,  double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_asinh,   OP_ASINH, double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_acosh,   OP_ACOSH, double, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_atanh,   OP_ATANH, double, double);

PASS_THROUGH_PTRFUNC_ONEARG(_INST_fpclassify,  OP_CLASS,  int, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isfinite,    OP_ISFIN,  int, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_finite,      OP_ISFIN,  int, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isnormal,    OP_ISNORM, int, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isnan,       OP_ISNAN,  int, double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isinf,       OP_ISINF,  int, double);

PASS_THROUGH_PTRFUNC_TWOARGS(_INST_atan2,     OP_ATAN2,    double, double, double);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_copysign,  OP_COPYSIGN, double, double, double);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_fmod,      OP_FMOD,     double, double, double);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_pow,       OP_POW,      double, double, double);

PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_modf,  ModF,  double, double, double*)
PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_frexp, FrExp, double, double, int*)
PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_ldexp, LdExp, double, double, int)

extern "C" void _ORIG_INST_sincos(double, double*, double*) __attribute__((weak));
void _INST_sincos(double x, double *sin, double *cos)
{
    if (_INST_status != _INST_INACTIVE) {
        _ORIG_INST_sincos(x, sin, cos);
        return;
    }
    _INST_status = _INST_ACTIVE;
    CHECK_FPSTSW
    //__asm__ ("emms;");
    if (_INST_Main_PointerAnalysis) {
        _INST_Main_PointerAnalysis->handleReplacedFuncSinCos(x, sin, cos);
    }
    if (_INST_Main_InplaceAnalysis) {
        _INST_Main_InplaceAnalysis->handleReplacedFuncSinCos(x, sin, cos);
    }
    _INST_status = _INST_INACTIVE;
}

// }}}

// {{{ single-precision pass-throughs

PASS_THROUGH_PTRFUNC_ONEARG(_INST_fabsf,    OP_ABS,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_ceilf,    OP_CEIL,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_erff,     OP_ERF,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_erfcf,    OP_ERFC,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_expf,     OP_EXP,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_exp2f,    OP_EXP2,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_floorf,   OP_FLOOR, float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_logf,     OP_LOG,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_logbf,    OP_LOGB,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_log10f,   OP_LOG10, float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_sqrtf,    OP_SQRT,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cbrtf,    OP_CBRT,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_truncf,   OP_TRUNC, float, float);

PASS_THROUGH_PTRFUNC_ONEARG(_INST_sinf,     OP_SIN,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cosf,     OP_COS,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_tanf,     OP_TAN,   float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_asinf,    OP_ASIN,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_acosf,    OP_ACOS,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_atanf,    OP_ATAN,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_sinhf,    OP_SINH,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_coshf,    OP_COSH,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_tanhf,    OP_TANH,  float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_asinhf,   OP_ASINH, float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_acoshf,   OP_ACOSH, float, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_atanhf,   OP_ATANH, float, float);

PASS_THROUGH_PTRFUNC_ONEARG(_INST_fpclassifyf,  OP_CLASS,  int, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isfinitef,    OP_ISFIN,  int, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_finitef,      OP_ISFIN,  int, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isnormalf,    OP_ISNORM, int, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isnanf,       OP_ISNAN,  int, float);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isinff,       OP_ISINF,  int, float);

PASS_THROUGH_PTRFUNC_TWOARGS(_INST_atan2f,     OP_ATAN2,    float, float, float);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_copysignf,  OP_COPYSIGN, float, float, float);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_fmodf,      OP_FMOD,     float, float, float);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_powf,       OP_POW,      float, float, float);

PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_modff,  ModF,  float, float, float*)
PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_frexpf, FrExp, float, float, int*)
PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_ldexpf, LdExp, float, float, int)

extern "C" void _ORIG_INST_sincosf(float, float*, float*) __attribute__((weak));
void _INST_sincosf(float x, float *sin, float *cos)
{
    if (_INST_status != _INST_INACTIVE) {
        _ORIG_INST_sincosf(x, sin, cos);
        return;
    }
    _INST_status = _INST_ACTIVE;
    CHECK_FPSTSW
    //__asm__ ("emms;");
    if (_INST_Main_PointerAnalysis) {
        _INST_Main_PointerAnalysis->handleReplacedFuncSinCos(x, sin, cos);
    }
    if (_INST_Main_InplaceAnalysis) {
        _INST_Main_InplaceAnalysis->handleReplacedFuncSinCos(x, sin, cos);
    }
    _INST_status = _INST_INACTIVE;
}

// }}}

// {{{ extended double-precision pass-throughs

PASS_THROUGH_PTRFUNC_ONEARG(_INST_fabsl,    OP_ABS,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_ceill,    OP_CEIL,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_erfl,     OP_ERF,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_erfcl,    OP_ERFC,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_expl,     OP_EXP,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_exp2l,    OP_EXP2,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_floorl,   OP_FLOOR, long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_logl,     OP_LOG,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_logbl,    OP_LOGB,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_log10l,   OP_LOG10, long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_sqrtl,    OP_SQRT,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cbrtl,    OP_CBRT,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_truncl,   OP_TRUNC, long double, long double);

PASS_THROUGH_PTRFUNC_ONEARG(_INST_sinl,     OP_SIN,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_cosl,     OP_COS,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_tanl,     OP_TAN,   long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_asinl,    OP_ASIN,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_acosl,    OP_ACOS,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_atanl,    OP_ATAN,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_sinhl,    OP_SINH,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_coshl,    OP_COSH,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_tanhl,    OP_TANH,  long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_asinhl,   OP_ASINH, long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_acoshl,   OP_ACOSH, long double, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_atanhl,   OP_ATANH, long double, long double);

PASS_THROUGH_PTRFUNC_ONEARG(_INST_fpclassifyl,  OP_CLASS,  int, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isfinitel,    OP_ISFIN,  int, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_finitel,      OP_ISFIN,  int, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isnormall,    OP_ISNORM, int, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isnanl,       OP_ISNAN,  int, long double);
PASS_THROUGH_PTRFUNC_ONEARG(_INST_isinfl,       OP_ISINF,  int, long double);

PASS_THROUGH_PTRFUNC_TWOARGS(_INST_atan2l,     OP_ATAN2,    long double, long double, long double);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_copysignl,  OP_COPYSIGN, long double, long double, long double);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_fmodl,      OP_FMOD,     long double, long double, long double);
PASS_THROUGH_PTRFUNC_TWOARGS(_INST_powl,       OP_POW,      long double, long double, long double);

PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_modfl,  ModF,  long double, long double, long double*)
PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_frexpl, FrExp, long double, long double, int*)
PASS_THROUGH_PTRFUNC_TWOARGS_SPECIAL(_INST_ldexpl, LdExp, long double, long double, int)

extern "C" void _ORIG_INST_sincosl(long double, long double*, long double*) __attribute__((weak));
void _INST_sincosl(long double x, long double *sin, long double *cos)
{
    if (_INST_status != _INST_INACTIVE) {
        _ORIG_INST_sincosl(x, sin, cos);
        return;
    }
    _INST_status = _INST_ACTIVE;
    CHECK_FPSTSW
    //__asm__ ("emms;");
    if (_INST_Main_PointerAnalysis) {
        _INST_Main_PointerAnalysis->handleReplacedFuncSinCos(x, sin, cos);
    }
    if (_INST_Main_InplaceAnalysis) {
        _INST_Main_InplaceAnalysis->handleReplacedFuncSinCos(x, sin, cos);
    }
    _INST_status = _INST_INACTIVE;
}

int _INST_fprintf(FILE* fd, const char *fmt, ...) {
    if (_INST_status != _INST_INACTIVE) {
        va_list args;
        va_start(args, fmt);
        return vfprintf(fd, fmt, args);
    }
    _INST_status = _INST_ACTIVE;
    //puts("_INST_fprintf: "); puts(fmt); puts ("\n");
#define PRINT_ORIG fprintf
#define PRINT_VORIG vfprintf
#define PRINT_OUT fd
#include "fprintf.cpp"
#undef PRINT_ORIG
#undef PRINT_VORIG
#undef PRINT_OUT
    _INST_status = _INST_INACTIVE;
    return ret;
}

int _INST_printf(const char *fmt, ...) {
    if (_INST_status != _INST_INACTIVE) {
        va_list args;
        va_start(args, fmt);
        return vprintf(fmt, args);
    }
    _INST_status = _INST_ACTIVE;
    //puts("_INST_printf: "); puts(fmt); puts ("\n");
#define PRINT_ORIG fprintf
#define PRINT_VORIG vfprintf
#define PRINT_OUT stdout
#include "fprintf.cpp"
#undef PRINT_ORIG
#undef PRINT_VORIG
#undef PRINT_OUT
    _INST_status = _INST_INACTIVE;
    return ret;
}

int _INST_sprintf(char *str, const char *fmt, ...) {
    if (_INST_status != _INST_INACTIVE) {
        va_list args;
        va_start(args, fmt);
        return vsprintf(str, fmt, args);
    }
    _INST_status = _INST_ACTIVE;
    //puts("_INST_sprintf(\""); puts(fmt); puts("\")\n");
#define PRINT_ORIG sprintf
#define PRINT_VORIG vsprintf
#define PRINT_OUT str
#define PRINT_INC str
#include "fprintf.cpp"
#undef PRINT_ORIG
#undef PRINT_VORIG
#undef PRINT_OUT
#undef PRINT_INC
    (*str++) = '\0';
    _INST_status = _INST_INACTIVE;
    return ret;
}

// }}}

#if 0
unsigned long _INST_fast_handle_pointer(long iidx,
        unsigned long eflags)
{
    //cout << hex << "esp: 0x" << esp << "  ebp: 0x" << ebp << "  flags: 0x" << eflags << dec << endl;
    __asm__ ("fxsave %0;" : : "m" (*mainContext->fxsave_state));
    //__asm__ ("movups %%xmm0, %0;" : : "m" (mainContext->fxsave_state->xmm_space[0]));
    //__asm__ ("movups %%xmm1, %0;" : : "m" (mainContext->fxsave_state->xmm_space[4]));
    //__asm__ ("movups %%xmm2, %0;" : : "m" (mainContext->fxsave_state->xmm_space[8]));
    CHECK_FPSTSW
    //mainContext->fxsave_state->fsw &= 0x3800; // TODO: re-enable?
    //__asm__ ("emms;");
    //_INST_print_flags("saved", mainContext->flags);
#if 0
    mainContext->reg_eax = eax; mainContext->reg_ebx = ebx;
    mainContext->reg_ecx = ecx; mainContext->reg_edx = edx;
    mainContext->reg_esp = esp /*+sizeof(unsigned long)*/; mainContext->reg_ebp = ebp;
    mainContext->reg_esi = esi; mainContext->reg_edi = edi;
#endif
    mainContext->flags = eflags;
    _INST_status = _INST_ACTIVE;

    // BEGIN INST
    FPSemantics *inst = mainDecoder->lookup(iidx);
#if 0
    mainContext->reg_eip = (unsigned long)inst->getAddress() + (unsigned long)inst->getNumBytes();
#endif
    //printf("zf: %lx  pf: %lx  cf: %lx\n", zf, pf, cf);
    //cout << "reg_eip = " << hex << mainContext->reg_eip << dec << endl;
    if (_INST_Main_PointerAnalysis) {
        _INST_Main_PointerAnalysis->handleInstruction(inst);
    }
    _INST_fast_count++;
    // END INST

    _INST_status = _INST_INACTIVE;
    //_INST_print_flags("restoring", mainContext->flags);
    __asm__ ("fxrstor %0;" : : "m" (*mainContext->fxsave_state));
    //__asm__ ("movups %0, %%xmm0;" : : "m" (mainContext->fxsave_state->xmm_space[0]));
    //__asm__ ("movups %0, %%xmm1;" : : "m" (mainContext->fxsave_state->xmm_space[4]));
    //__asm__ ("movups %0, %%xmm2;" : : "m" (mainContext->fxsave_state->xmm_space[8]));
    return mainContext->flags;
}
#endif

extern "C"
void _INST_fortran_report(const double *val, const char *lbl, const int lbl_len) {
    static char buffer[1024];
    //static int count = 0;
    _INST_status = _INST_ACTIVE;
    void* a_ptr = *(void**)(val);
    double a_dbl = *(double*)(val);
    strncpy(buffer, lbl, lbl_len);
    buffer[lbl_len] = '\0';
    if (_INST_Main_PointerAnalysis && _INST_Main_PointerAnalysis->isSVPtr(a_ptr)) {
        printf("REPORT %s: %.8g [%p] => ^%.8g\n", buffer, *val, a_ptr,
                ((FPSV*)a_ptr)->getValue(IEEE_Double).data.dbl);
    } else if (_INST_Main_InplaceAnalysis && _INST_Main_InplaceAnalysis->isReplaced(a_dbl)) {
        printf("REPORT %s: %.8g [%p] => ^%.8g\n", buffer, *val, a_ptr,
                _INST_Main_InplaceAnalysis->extractFloat(a_dbl));
    } else {
        printf("REPORT %s: %.8g\n", buffer, *val);
    }
    _INST_status = _INST_INACTIVE;
}

/*
 *void _INST_print_double(const char *fmt, double val) {
 *    double rval = _INST_get_double(val);
 *    printf(fmt, rval);
 *}
 *
 *void _INST_get_double(double val) {
 *    double ret = 0.0/0.0;
 *    if (mainPtrDoubleAnalysis) {
 *        mainPtrDoubleAnalysis->shadowWriteValue(&ret, (void*)(&val));
 *    }
 *    return ret;
 *}
 */

void _INST_handle_unsupported_inst(long iidx)
{
    _INST_status = _INST_ACTIVE;
    FPSemantics *inst = mainDecoder->lookup(iidx);
    fprintf(stderr, "ERROR - unhandled runtime instruction at %p: %s\n",
           inst->getAddress(), inst->getDisassembly().c_str());
    _INST_status = _INST_INACTIVE;
}

long _INST_get_analysis_id(FPAnalysis *analysis)
{
    string tag = analysis->getTag();
    long id = 0;
    if (tag == "cinst") {
        id = cinstID;
    } else if (tag == "dcancel") {
        id = dcancelID;
    } else if (tag == "dnan") {
        id = dnanID;
    } else if (tag == "trange") {
        id = trangeID;
    } else if (tag == "svptr") {
        id = svptrID;
    } else if (tag == "svinp") {
        id = svinpID;
    }
    return id;
}

void _INST_handle_pre_analysis(long analysisID, long iidx)
{
    FPAnalysis *analysis = NULL;
    switch (analysisID) {
        case cinstID:   analysis = _INST_Main_CInstAnalysis;        break;
        case dcancelID: analysis = _INST_Main_DCancelAnalysis;      break;
        case dnanID:    analysis = _INST_Main_DNanAnalysis;         break;
        case trangeID:  analysis = _INST_Main_TRangeAnalysis;       break;
        case svptrID:   analysis = _INST_Main_PointerAnalysis;      break;
        case svinpID:   analysis = _INST_Main_InplaceAnalysis;      break;
        default:        assert(!"Invalid analysis ID");             break;
    }
    __asm__ ("fxsave %0;" : : "m" (*mainContext->fxsave_state));
    _INST_status = _INST_ACTIVE;
    analysis->handlePreInstruction(mainDecoder->lookup(iidx));
    _INST_status = _INST_INACTIVE;
    __asm__ ("fxrstor %0;" : : "m" (*mainContext->fxsave_state));
}

void _INST_handle_post_analysis(long analysisID, long iidx)
{
    FPAnalysis *analysis = NULL;
    switch (analysisID) {
        case cinstID:   analysis = _INST_Main_CInstAnalysis;        break;
        case dcancelID: analysis = _INST_Main_DCancelAnalysis;      break;
        case dnanID:    analysis = _INST_Main_DNanAnalysis;         break;
        case trangeID:  analysis = _INST_Main_TRangeAnalysis;       break;
        case svptrID:   analysis = _INST_Main_PointerAnalysis;      break;
        case svinpID:   analysis = _INST_Main_InplaceAnalysis;      break;
        default:        assert(!"Invalid analysis ID");             break;
    }
    __asm__ ("fxsave %0;" : : "m" (*mainContext->fxsave_state));
    _INST_status = _INST_ACTIVE;
    analysis->handlePostInstruction(mainDecoder->lookup(iidx));
    _INST_status = _INST_INACTIVE;
    __asm__ ("fxrstor %0;" : : "m" (*mainContext->fxsave_state));
}

void _INST_handle_replacement(long analysisID, long iidx)
{
    FPAnalysis *analysis = NULL;
    switch (analysisID) {
        case cinstID:   analysis = _INST_Main_CInstAnalysis;        break;
        case dcancelID: analysis = _INST_Main_DCancelAnalysis;      break;
        case dnanID:    analysis = _INST_Main_DNanAnalysis;         break;
        case trangeID:  analysis = _INST_Main_TRangeAnalysis;       break;
        case svptrID:   analysis = _INST_Main_PointerAnalysis;      break;
        case svinpID:   analysis = _INST_Main_InplaceAnalysis;      break;
        default:        assert(!"Invalid analysis ID");             break;
    }
    __asm__ ("fxsave %0;" : : "m" (*mainContext->fxsave_state));
    _INST_status = _INST_ACTIVE;
    analysis->handleReplacement(mainDecoder->lookup(iidx));
    _INST_status = _INST_INACTIVE;
    __asm__ ("fxrstor %0;" : : "m" (*mainContext->fxsave_state));
}

void _INST_disable_analysis ()
{
    _INST_status = _INST_ACTIVE;
    printf("FPAnalysis: Profiler disabled.\n");
    _INST_status = _INST_DISABLED;
}

void _INST_cleanup_analysis ()
{
    size_t i;
    stringstream msg;

    mainContext->saveAllFPR();
    _INST_status = _INST_ACTIVE;

    if (mainConfig->getValue("enable_profiling") == "yes") {
        struct itimerval t;
        t.it_value.tv_sec = 0;
        t.it_value.tv_usec = 0;
        t.it_interval.tv_sec = 0;
        t.it_interval.tv_usec = 0;
        setitimer(ITIMER_VIRTUAL, &t, 0);
    }

    cout << "FPAnalysis: Cleanup in process ..." << endl;

    //printf("finalizing %d analyses...\n", analysisCount);
    // final analysis output
    for (i=0; i<analysisCount; i++) {
        allAnalyses[i]->finalOutput();
    }
    //printf("done finalizing analyses\n");
    //fflush(stdout);

    // finalize the logfile
    msg.clear();
    msg.str("");
    msg << "Full analysis: " << _INST_count << " instruction(s) handled" << endl;
    msg << "Optimized analysis: " << _INST_fast_count << " instruction(s) handled";
    mainLog->addMessage(STATUS, 0, "Profiling finished.", msg.str(), "");
    mainLog->close();
    printf("%s\n", msg.str().c_str());

    cout << "FPAnalysis: Full analysis: " << _INST_count << " instruction(s) handled" << endl;
    cout << "FPAnalysis: Optimized analysis: " << _INST_fast_count << " instruction(s) handled" << endl;
    cout << "FPAnalysis: Log written to " << _INST_log_file << endl;

    _INST_status = _INST_DISABLED;
    mainContext->restoreAllFPR();

    return;

    // clean up memory
    // TODO: fix all this to work properly
    delete mainConfig;
    delete mainContext;
    delete mainDecoder;
    delete mainLog;
}

/* needed to keep some compilers happy? */
/*
int main()
{
    return 0;
}
*/

