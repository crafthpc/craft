#ifndef __FPCONFIG_H
#define __FPCONFIG_H

#include <ctype.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>

#include "FPShadowEntry.h"
#include "FPReplaceEntry.h"

using namespace std;

namespace FPInst {

/**
 * Stores key-value pairs representing an analysis configuration.
 */
class FPConfig
{
    public:

        static FPConfig* getMainConfig();

        FPConfig();
        FPConfig(string filename);

        bool hasValue(string key);
        string getValue(string key);
        const char* getValueC(string key);

        void getAllSettings(vector<string> &vals);

        bool hasReplaceTagTree();
        FPReplaceEntryTag getReplaceTag(void *address);

        void getAllShadowEntries(vector<FPShadowEntry*> &entries);
        void getAllReplaceEntries(vector<FPReplaceEntry*> &entries);
        int getAddressList(void* addresses[], string key);
        string getSummary(bool includeReplace=false);
        void buildFile(ostream &out);

        void addSetting(char* line);
        void addSetting(string line);
        void addReplaceEntry(string line);
        void addReplaceEntry(FPReplaceEntry *entry);
        void setValue(string key, string value);
        void saveFile(string filename);

    private:


        /**
         * MASTER LIST OF CONFIG FILE FLAGS
         *
         * These used to be only for replacement (thus the "RE_" nomenclature),
         * but now they can be used to configure a mutatee for all types of analysis.
         */
        static const char RE_FLAG      = '^';       // flag indicating a mutatee config entry
        static const char RE_NONE      = ' ';       // empty flag
        static const char RE_IGNORE    = '!';       // ignore (do not instrument or replace)
        static const char RE_NULL      = 'x';       // null instrumentation (only Dyninst overhead)

        // Replacement-based analyses:
        static const char RE_TRANGE    = 'r';       // range tracking replacement
        static const char RE_CANDIDATE = '?';       // candidate for replacement
        static const char RE_SINGLE    = 's';       // single-precision replacement
        static const char RE_DOUBLE    = 'd';       // double-precision replacement

        // Instrumentation-based analyses:
        static const char RE_CINST     = 'i';       // instruction counting
        static const char RE_DCANCEL   = 'c';       // cancellation detection
        static const char RE_DNAN      = 'n';       // NaN detection


        static const string RE_APP;
        static const string RE_MODULE;
        static const string RE_FUNCTION;
        static const string RE_BASICBLOCK;
        static const string RE_INSTRUCTION;

        static FPConfig* singletonConfig;

        FPReplaceEntry *currentApp;
        FPReplaceEntry *currentModule;
        FPReplaceEntry *currentFunction;
        FPReplaceEntry *currentBasicBlock;

        string getShadowEntryLine(FPShadowEntry *entry);
        string getReplaceEntryLine(FPReplaceEntry *rentry);
        void buildSummary(ostream &out, bool includeReplace=true);

        map<string, string> settings;
        vector<FPShadowEntry*> shadowEntries;
        vector<FPReplaceEntry*> replaceEntries;
        map<void*, FPReplaceEntry*> instructionEntries;
};

}

#endif
