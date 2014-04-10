/*
 * libfpanalysis.cpp
 *
 * Floating-point analysis library
 *
 * Originally written by Mike Lam, Fall 2008
 */

//#include "fpanalysis.h"
#include "fpflag.h"
#include "fpinfo.h"

// standard C/Unix libs
#include <unistd.h>

// helpers
#include "FPContext.h"
#include "FPConfig.h"
#include "FPLog.h"
#include "FPDecoderXED.h"
#include "FPDecoderIAPI.h"

using namespace FPInst;

FPContext *mainContext;
FPConfig  *mainConfig;
FPLog     *mainLog;
FPDecoder *mainDecoder;

FPAnalysis *mainNullAnalysis;
FPAnalysis *allAnalyses[TOTAL_ANALYSIS_COUNT];

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

void _INST_set_config_replace_entry (size_t idx, void* address, int type, int tag)
{
    _INST_status = _INST_ACTIVE;
    FPReplaceEntry *entry = new FPReplaceEntry((FPReplaceEntryType)type, idx);
    entry->address = address;
    entry->tag = (FPReplaceEntryTag)tag;
    //printf("adding replace entry: %s\n", entry->toString().c_str());
    FPConfig::getMainConfig()->addReplaceEntry(entry);
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
    
    // initialize various analyses
    analysisCount = 0;
    for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        string tag = allAnalysisInfo[aidx].instance->getTag();
        if (mainConfig->hasValue(tag) && mainConfig->getValue(tag) == "yes") {
            allAnalysisInfo[aidx].instance->configure(mainConfig, mainDecoder, mainLog, mainContext);
            allAnalyses[analysisCount++] = allAnalysisInfo[aidx].instance;
            status << tag << ": initialized" << endl;
        }
    }
    if (analysisCount == 0) {
        // TODO: revisit this (should null analysis be in allAnalysisInfo?)
        mainNullAnalysis = new FPAnalysis();
        allAnalyses[analysisCount++] = mainNullAnalysis;
        status << "null instrumentation initialized" << endl;
    }

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
    for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        if (allAnalysisInfo[aidx].instance->getTag() == analysis->getTag()) {
            return (long)aidx;
        }
    }
    return 0;
}

void _INST_handle_pre_analysis(long analysisID, long iidx)
{
    assert(analysisID >=0  && analysisID < (long)TOTAL_ANALYSIS_COUNT);
    FPAnalysis *analysis = allAnalysisInfo[analysisID].instance;
    __asm__ ("fxsave %0;" : : "m" (*mainContext->fxsave_state));
    _INST_status = _INST_ACTIVE;
    analysis->handlePreInstruction(mainDecoder->lookup(iidx));
    _INST_status = _INST_INACTIVE;
    __asm__ ("fxrstor %0;" : : "m" (*mainContext->fxsave_state));
}

void _INST_handle_post_analysis(long analysisID, long iidx)
{
    assert(analysisID >=0  && analysisID < (long)TOTAL_ANALYSIS_COUNT);
    FPAnalysis *analysis = allAnalysisInfo[analysisID].instance;
    __asm__ ("fxsave %0;" : : "m" (*mainContext->fxsave_state));
    _INST_status = _INST_ACTIVE;
    analysis->handlePostInstruction(mainDecoder->lookup(iidx));
    _INST_status = _INST_INACTIVE;
    __asm__ ("fxrstor %0;" : : "m" (*mainContext->fxsave_state));
}

void _INST_handle_replacement(long analysisID, long iidx)
{
    assert(analysisID >=0  && analysisID < (long)TOTAL_ANALYSIS_COUNT);
    FPAnalysis *analysis = allAnalysisInfo[analysisID].instance;
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

