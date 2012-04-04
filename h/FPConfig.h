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

        FPReplaceEntryTag getReplaceTag(void *address);

        void getAllShadowEntries(vector<FPShadowEntry*> &entries);
        int getAddressList(void* addresses[], string key);
        string getSummary(bool includeReplace=false);

        void addSetting(char* line);
        void addSetting(string line);
        void addReplaceEntry(string line);
        void addReplaceEntry(FPReplaceEntry *entry);
        void setValue(string key, string value);
        void saveFile(string filename);

    private:

        static const char RE_FLAG = '^';
        static const char RE_NONE = ' ';
        static const char RE_IGNORE = 'i';
        static const char RE_SINGLE = 's';
        static const char RE_DOUBLE = 'd';
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
