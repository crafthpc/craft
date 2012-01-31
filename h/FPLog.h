#ifndef __FPLOG_H
#define __FPLOG_H

#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>

#include "time.h"

// StackwalkerAPI and SymtabAPI (needed for error traces)
#include "stackwalk/h/walker.h"
#include "stackwalk/h/framestepper.h"
#include "stackwalk/h/steppergroup.h"
#include "stackwalk/h/frame.h"
#include "Symtab.h"
#include "Type.h"
#include "Function.h"
#include "Variable.h"
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;
using namespace Dyninst::Stackwalker;

#include "FPSemantics.h"

using namespace std;

namespace FPInst {

enum FPLogMsgType {
    STATUS, ERROR, WARNING, SUMMARY, CANCELLATION, ICOUNT, SHVALUE
};

/**
 * Handles log file I/O.
 * Can serialize messages to an output file, including any desired stack traces
 * or debug information.
 */
class FPLog
{
    public:

        static string msgType2Str(FPLogMsgType type);

        FPLog(string filename);
        FPLog(string filename, string appname);

        void setDebugFile(string filename);

        void enableStackWalks();
        void disableStackWalks();
        bool isStackWalkingEnabled();

        void addMessage(FPLogMsgType type, long priority, 
                string label, string details, string trace,
                FPSemantics *inst=NULL);

        string getStackTrace(unsigned skipFrames=5);
        string getFakeStackTrace(void *addr);

        void getGlobalVars(vector<Variable *> &gvars);
        FPOperandAddress getGlobalVarAddr(string name);
        string getGlobalVarName(FPOperandAddress addr);
        unsigned getGlobalVarSize(string name);
        unsigned getGlobalVarSize(FPOperandAddress addr);
        Type* getGlobalVarType(string name);
        Type* getGlobalVarType(FPOperandAddress addr);

        void close();

    private:

        string sanitize(const string &text);

        void writeTraces();
        void writeInstructions();

        bool fileOpen;
        ofstream logfile;
        map<string, unsigned> traces;
        map<FPSemantics *, unsigned> instructions;

        // stack walking stuff
        Walker *walker;
        vector<Frame> stackwalk; 
        string stwalk_func; 
        string stwalk_lib; 
        Offset stwalk_offset;
        Symtab *stwalk_symtab;
        vector<Statement*> stwalk_lines;
        bool stwalk_enabled;

        // debug information
        Symtab *debug_symtab;
};

}

#endif

