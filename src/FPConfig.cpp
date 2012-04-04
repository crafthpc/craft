#include "FPConfig.h"

namespace FPInst {

const string FPConfig::RE_APP = "APPLICATION";
const string FPConfig::RE_MODULE = "MODULE";
const string FPConfig::RE_FUNCTION = "FUNC";
const string FPConfig::RE_BASICBLOCK = "BBLK";
const string FPConfig::RE_INSTRUCTION = "INSN";

FPConfig* FPConfig::singletonConfig = NULL;

FPConfig* FPConfig::getMainConfig() {
    if (!singletonConfig) {
        singletonConfig = new FPConfig();
    }
    return singletonConfig;
}

FPConfig::FPConfig()
{
    currentApp = NULL;
    currentModule = NULL;
    currentFunction = NULL;
    currentBasicBlock = NULL;
}

FPConfig::FPConfig(string filename)
{
    currentApp = NULL;
    currentModule = NULL;
    currentFunction = NULL;
    currentBasicBlock = NULL;
    ifstream fin(filename.c_str());
    string line, key, value, type;
    while (!fin.fail() && getline(fin, line)) {
        addSetting(line);
    }
    fin.close();
}

void FPConfig::addSetting(char* line) {
    addSetting(string(line));
}

void FPConfig::addSetting(string line) {
    static FPShadowEntry *newEntry = NULL;
    string key, value, type;
    size_t pos, pos2;
    size_t isize = 0;
    if (line[0] == RE_FLAG) {
        addReplaceEntry(line);
        return;
    }
    pos = line.find_first_of('=');
    pos2 = line.find_first_of(':');
    if (pos2 != string::npos) {
        type = line.substr(0,pos2);
    } else {
        type = "regular";
        pos2 = -1;
    }
    if (pos != string::npos) {
        key = line.substr(pos2+1,(pos-pos2-1));
        value = line.substr(pos+1, line.length()-pos+1);
    } else {
        key = line.substr(pos2+1, line.length()-pos2-1);
        value = "";
    }
    pos = type.find_first_of('-');
    if (pos != string::npos) {
        isize = atol(type.substr(pos+1,type.length()-pos-1).c_str());
        type = type.substr(0,pos);
    }
    //cout << "::: type=" << type << " isize=" << isize << " key=\"" << key << "\" value=\"" << value << "\"" << endl;
    if (type == "regular") {
        settings[key] = value;
        //cout << "  regular setting: " << key << "=" << value << endl;
    } else if (type == "init") {
        newEntry = new FPShadowEntry();
        newEntry->type = INIT;
        newEntry->name = key;
        newEntry->isize = isize;
        pos = 0;
        while (value[pos] == ' ') pos++;
        pos2 = value.find_first_of(' ', pos);
        while (pos2 != string::npos) {
            newEntry->vals.push_back(value.substr(pos,pos2-pos));
            pos = pos2+1;
            while (value[pos] == ' ') pos++;
            pos2 = value.find_first_of(' ', pos);
        }
        if (/*pos >= 0 &&*/ pos < value.length()-1) {
            newEntry->vals.push_back(value.substr(pos,value.length()-pos+1));
        }
        newEntry->size[0] = newEntry->vals.size();
        newEntry->size[1] = 1;

        //cout << "  shadow value init: name=" << key << " vals=[ ";
        //vector<string>::iterator i;
        //for (i=newEntry->vals.begin(); i!=newEntry->vals.end(); i++) {
            //cout << (*i) << " ";
        //}
        //cout << "] size=(" << newEntry->size[0] << "," << newEntry->size[1] << ")" << endl;
    } else if (type == "report") {
        newEntry = new FPShadowEntry();
        newEntry->type = REPORT;
        newEntry->name = key;
        newEntry->isize = isize;
        pos = value.find_first_of(' ');
        if (pos != string::npos) {
            newEntry->size[0] = (unsigned long)atol(value.substr(0,pos).c_str());
            newEntry->size[1] = (unsigned long)atol(value.substr(pos+1,value.length()-pos+1).c_str());
        } else {
            newEntry->size[0] = (unsigned long)atol(value.c_str());
            newEntry->size[1] = (newEntry->size[0] > 0 ? 1 : 0);
        }

       //cout << "  shadow value report: name=" << newEntry->name << " size=(" << newEntry->size[0] << "," << newEntry->size[1] << ")" << endl;
    } else if (type == "noreport") {
        newEntry = new FPShadowEntry();
        newEntry->type = NOREPORT;
        newEntry->name = key;
    }

    if (newEntry) {
        if (newEntry->name.length() > 0 && newEntry->name[0] == '*') {
            newEntry->indirect = true;
            newEntry->name = newEntry->name.erase(0,1);
            //cout << "    indirect - name:" << newEntry->name << endl;
        } else {
            newEntry->indirect = false;
        }
        shadowEntries.push_back(newEntry);
        newEntry = NULL;
    }
}

void FPConfig::addReplaceEntry(string line)
{
    //printf("adding setting %s\n", line.c_str());
    FPReplaceEntry *entry = new FPReplaceEntry();
    size_t pos, len;

    // parse tag
    if (line.length() >= 2) {
        switch (line[1]) {
            case RE_IGNORE:     entry->tag = RETAG_IGNORE;      break;
            case RE_DOUBLE:     entry->tag = RETAG_DOUBLE;      break;
            case RE_SINGLE:     entry->tag = RETAG_SINGLE;      break;
            default:            entry->tag = RETAG_NONE;        break;
        }
    }

    // parse type
    if (line.find(RE_APP) != string::npos) {
        entry->type = RETYPE_APP;
        currentApp = entry;
    } else if (line.find(RE_MODULE) != string::npos) {
        entry->type = RETYPE_MODULE;
        entry->parent = currentApp;
        currentModule = entry;
    } else if (line.find(RE_FUNCTION) != string::npos) {
        entry->type = RETYPE_FUNCTION;
        entry->parent = currentModule;
        currentFunction = entry;
    } else if (line.find(RE_BASICBLOCK) != string::npos) {
        entry->type = RETYPE_BASICBLOCK;
        entry->parent = currentFunction;
        currentBasicBlock = entry;
    } else if (line.find(RE_INSTRUCTION) != string::npos) {
        entry->type = RETYPE_INSTRUCTION;
        entry->parent = currentBasicBlock;
    }

    // parse id
    pos = line.find('#');
    if (pos != string::npos) {
        pos += 1;
        len = 1;
        while (isdigit(line[pos+len])) {
            len++;
        }
        entry->idx = atoi(line.substr(pos,len).c_str());
    }

    // parse address
    pos = line.find("0x");
    if (pos != string::npos) {
        pos += 2;
        len = 1;
        while (isxdigit(line[pos+len])) {
            len++;
        }
        /*
         *cout << "id " << entry->idx << " pos=" << pos << " len=" 
         *     << len << " str=\"" << line.substr(pos,len) << "\"" 
         *     << "  " << line << endl;
         */
        entry->address = (void*)strtol(line.substr(pos,len).c_str(), NULL, 16);
    }

    // parse name
    pos = line.find('"');
    if (pos != string::npos) {
        pos += 1;
        len = 0;
        while (line[pos+len] != '"') {
            len++;
        }
        entry->name = line.substr(pos,len);
    }

    //cout << entry->toString() << endl;
    addReplaceEntry(entry);
}

void FPConfig::addReplaceEntry(FPReplaceEntry *entry)
{
    replaceEntries.push_back(entry);
    if (entry->type == RETYPE_INSTRUCTION) {
        instructionEntries[entry->address] = entry;
    }
}

bool FPConfig::hasValue(string key)
{
    return settings.find(key) != settings.end();
}

string FPConfig::getValue(string key)
{
    map<string,string>::iterator i = settings.find(key);
    if (i == settings.end())
        return "";
    else
        return i->second;
}

const char* FPConfig::getValueC(string key)
{
    map<string,string>::iterator i = settings.find(key);
    if (i == settings.end())
        return NULL;
    else
        return i->second.c_str();
}

void FPConfig::getAllSettings(vector<string> &vals)
{
    stringstream ss;
    map<string,string>::iterator i;
    for (i=settings.begin(); i!=settings.end(); i++) {
        ss.clear(); ss.str("");
        ss << i->first << "=" << i->second;
        vals.push_back(ss.str());
    }
    vector<FPShadowEntry*>::iterator j;
    for (j=shadowEntries.begin(); j!=shadowEntries.end(); j++) {
        vals.push_back(getShadowEntryLine(*j));
    }
    vector<FPReplaceEntry*>::iterator jj;
    for (jj=replaceEntries.begin(); jj!=replaceEntries.end(); jj++) {
        vals.push_back(getReplaceEntryLine(*jj));
    }
}

FPReplaceEntryTag FPConfig::getReplaceTag(void *address)
{
    FPReplaceEntryTag tag = RETAG_NONE;
    FPReplaceEntry *ientry = instructionEntries[address];
    if (ientry != NULL) {
        tag = ientry->getEffectiveTag();
    }
    return tag;
}

void FPConfig::getAllShadowEntries(vector<FPShadowEntry*> &entries)
{
    vector<FPShadowEntry*>::iterator i;
    for (i=shadowEntries.begin(); i!=shadowEntries.end(); i++)
        entries.push_back(*i);
}

int FPConfig::getAddressList(void* addresses[], string key)
{
    int count = 0;
    char addrs[2048], *addr = NULL;
    unsigned long val;
    strncpy(addrs, getValueC(key), 2048);
    addr = strtok(addrs, " ");
    while (addr != NULL) {
        sscanf(addr, "%lx", &val);
        addresses[count++] = ((void*)val);
        //cout << " address " << hex << val << dec << endl;
        addr = strtok(NULL, " ");
    }
    return count;
}

string FPConfig::getShadowEntryLine(FPShadowEntry *entry)
{
    stringstream ss;
    ss.clear(); ss.str("");
    ss << "  ";
    switch (entry->type) {
        case INIT:     ss << "init";     break;
        case REPORT:   ss << "report";   break;
        case NOREPORT: ss << "noreport"; break;
    }
    if (entry->isize != 0) {
        ss << "-" << entry->isize;
    }
    ss << ":" << (entry->indirect ? "*" : "") << entry->name << "=";
    if (entry->type == INIT) {
        vector<string>::iterator k;
        for (k=entry->vals.begin(); k!=entry->vals.end(); k++) {
            if (k != entry->vals.begin())
                ss << " ";
            ss << *k;
        }
    } else {
        ss << entry->size[0] << " " << entry->size[1];
    }
    return ss.str();
}

string FPConfig::getReplaceEntryLine(FPReplaceEntry *rentry)
{
    stringstream ss;
    ss.clear(); ss.str("");
    ss << RE_FLAG;
    switch (rentry->tag) {
        case RETAG_IGNORE:      ss << RE_IGNORE;      break;
        case RETAG_DOUBLE:      ss << RE_DOUBLE;      break;
        case RETAG_SINGLE:      ss << RE_SINGLE;      break;
        default:                ss << RE_NONE;      break;
    }
    ss << " ";
    switch (rentry->type) {
        case RETYPE_APP:        ss << RE_APP;
                                currentApp = rentry;        break;
        case RETYPE_MODULE:     ss << "  " << RE_MODULE;
                                currentModule = rentry;     break;
        case RETYPE_FUNCTION:   ss << "    " << RE_FUNCTION;
                                currentFunction = rentry;   break;
        case RETYPE_BASICBLOCK: ss << "      " << RE_BASICBLOCK;
                                currentBasicBlock = rentry; break;
        case RETYPE_INSTRUCTION:ss << "        " << RE_INSTRUCTION; break;
    }
    ss << " #" << rentry->idx << ": " << hex << rentry->address << dec;
    if (rentry->name != "") {
        ss << " \"" << rentry->name << "\"";
    }
    return ss.str();
}

void FPConfig::buildSummary(ostream &out, bool includeReplace)
{
    map<string,string>::iterator i;
    for (i=settings.begin(); i!=settings.end(); i++) {
        if (i->first.find("max_addr") == string::npos && 
            i->first.find("min_addr") == string::npos &&
            i->first.find("count_addr") == string::npos) {
            out << "  " << i->first << "=" << i->second << endl;
        }
    }
    vector<FPShadowEntry*>::iterator j;
    for (j=shadowEntries.begin(); j!=shadowEntries.end(); j++) {
        out << getShadowEntryLine(*j) << endl;
    }
    if (includeReplace) {
        vector<FPReplaceEntry*>::iterator jj;
        for (jj=replaceEntries.begin(); jj!=replaceEntries.end(); jj++) {
            out << getReplaceEntryLine(*jj) << endl;
        }
    }
}

string FPConfig::getSummary(bool includeReplace)
{
    stringstream ss;
    ss.clear();
    ss.str("");
    buildSummary(ss, includeReplace);
    return ss.str();
}

void FPConfig::setValue(string key, string value)
{
    settings[key] = value;
}

void FPConfig::saveFile(string filename)
{
    ofstream fout(filename.c_str());
    buildSummary(fout);
    fout.close();
}

}
