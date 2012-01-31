#include "FPConfig.h"

namespace FPInst {

const string FPConfig::RE_APP = "APPLICATION";
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
    currentFunction = NULL;
    currentBasicBlock = NULL;
}

FPConfig::FPConfig(string filename)
{
    currentApp = NULL;
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
    } else if (line.find(RE_FUNCTION) != string::npos) {
        entry->type = RETYPE_FUNCTION;
        entry->parent = currentApp;
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

//map<string, string>::iterator FPConfig::begin() {
    //return settings.begin();
//}

//map<string, string>::iterator FPConfig::end() {
    //return settings.end();
//}

void FPConfig::getAllValues(map<string, string> &vals)
{
    //map<string,string>::iterator i;
    //for (i=settings.begin(); i!=settings.end(); i++)
        //vals[i->first] = i->second;
    map<string,string>::iterator i;
    for (i=settings.begin(); i!=settings.end(); i++)
        vals[i->first] = i->second;
    vector<FPShadowEntry*>::iterator j;
    vector<string>::iterator k;
    FPShadowEntry *entry;
    stringstream ss;
    string key, value;
    for (j=shadowEntries.begin(); j!=shadowEntries.end(); j++) {
        ss.clear();
        ss.str("");
        entry = *j;
        switch (entry->type) {
            case INIT:     ss << "init";     break;
            case REPORT:   ss << "report";   break;
            case NOREPORT: ss << "noreport"; break;
        }
        if (entry->isize != 0) {
            ss << "-" << entry->isize;
        }
        ss << ":" << (entry->indirect ? "*" : "") << entry->name;
        key = ss.str();
        ss.clear();
        ss.str("");
        if (entry->type == INIT) {
            for (k=entry->vals.begin(); k!=entry->vals.end(); k++) {
                if (k != entry->vals.begin())
                    ss << " ";
                ss << *k;
            }
        } else {
            ss << entry->size[0] << " " << entry->size[1];
        }
        value = ss.str();
        vals[key] = value;
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
    vector<string>::iterator k;
    FPShadowEntry *entry;
    for (j=shadowEntries.begin(); j!=shadowEntries.end(); j++) {
        entry = *j;
        out << "  ";
        switch (entry->type) {
            case INIT:     out << "init";     break;
            case REPORT:   out << "report";   break;
            case NOREPORT: out << "noreport"; break;
        }
        if (entry->isize != 0) {
            out << "-" << entry->isize;
        }
        out << ":" << (entry->indirect ? "*" : "") << entry->name << "=";
        if (entry->type == INIT) {
            for (k=entry->vals.begin(); k!=entry->vals.end(); k++) {
                if (k != entry->vals.begin())
                    out << " ";
                out << *k;
            }
        } else {
            out << entry->size[0] << " " << entry->size[1];
        }
        out << endl;
    }
    if (includeReplace) {
        vector<FPReplaceEntry*>::iterator jj;
        FPReplaceEntry *rentry;
        for (jj=replaceEntries.begin(); jj!=replaceEntries.end(); jj++) {
            rentry = *jj;
            out << RE_FLAG;
            switch (rentry->tag) {
                case RETAG_IGNORE:      out << RE_IGNORE;      break;
                case RETAG_DOUBLE:      out << RE_DOUBLE;      break;
                case RETAG_SINGLE:      out << RE_SINGLE;      break;
                default:                out << RE_NONE;      break;
            }
            out << " ";
            switch (rentry->type) {
                case RETYPE_APP:        out << RE_APP;
                                        currentApp = rentry;        break;
                case RETYPE_FUNCTION:   out << "  " << RE_FUNCTION;
                                        currentFunction = rentry;   break;
                case RETYPE_BASICBLOCK: out << "    " << RE_BASICBLOCK;
                                        currentBasicBlock = rentry; break;
                case RETYPE_INSTRUCTION:out << "      " << RE_INSTRUCTION; break;
            }
            out << " #" << rentry->idx << ": " << hex << rentry->address << dec;
            if (rentry->name != "") {
                out << " \"" << rentry->name << "\"";
            }
            out << endl;
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
