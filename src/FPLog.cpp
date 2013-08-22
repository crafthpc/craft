#include "FPLog.h"

namespace FPInst {

string FPLog::msgType2Str(FPLogMsgType type)
{
    switch (type) {
        case STATUS: return "Status";
        case ERROR: return "Error";
        case WARNING: return "Warning";
        case SUMMARY: return "Summary";
        case CANCELLATION: return "Cancellation";
        case ICOUNT: return "InstCount";
        case SHVALUE: return "ShadowVal";
        default: return "Unknown";
    }
}

FPLog::FPLog(string filename)
{
    logfile.open(filename.c_str(), ios::out);
    logfile << "<log>" << endl;
    fileOpen = true;
    stwalk_enabled = false;
    debug_symtab = NULL;
}

FPLog::FPLog(string filename, string appname)
{
    logfile.open(filename.c_str(), ios::out);
    logfile << "<log appname=\"" << appname << "\">" << endl;
    fileOpen = true;
    stwalk_enabled = false;
    debug_symtab = NULL;
}

void FPLog::enableStackWalks()
{
    stwalk_enabled = true;
    walker = Walker::newWalker(); 
    if (walker == NULL) {
        addMessage(ERROR, 0, "Cannot initialize stackwalker", "", "");
    } else {
        DyninstInstrStepper *instr_stepper = new DyninstInstrStepper(walker);
        walker->getStepperGroup()->registerStepper(instr_stepper);
    }
}

void FPLog::disableStackWalks()
{
    stwalk_enabled = false;
}

bool FPLog::isStackWalkingEnabled()
{
    return stwalk_enabled;
}

void FPLog::setDebugFile(string filename)
{
    Symtab::openFile(debug_symtab, filename);
}

void FPLog::addMessage(FPLogMsgType type, long priority, 
        string label, string details, string trace, FPSemantics *inst)
{
    if (!fileOpen) return;

    long id;
    long timestamp = (long)clock();

    logfile << "<message time=\"" << timestamp;
    logfile << "\" priority=\"" << priority;
    logfile << "\" type=\"" << msgType2Str(type) << "\">" << endl;

    if (label.length() > 0)
        logfile << "<label>" << label << "</label>" << endl;

    if (details.length() > 0)
        logfile << "<details>" << details << "</details>" << endl;

    if (trace.length() > 0) {
        map<string, unsigned>::iterator i = traces.find(trace);
        if (i == traces.end()) {
            id = traces.size()+1;
            traces[trace] = id;
        } else {
            id = i->second;
        }
        logfile << "<trace_id>" << id << "</trace_id>" << endl;
    }

    if (inst != NULL) {
        map<FPSemantics *, unsigned>::iterator i = instructions.find(inst);
        if (i == instructions.end()) {
            id = instructions.size()+1;
            instructions[inst] = id;
        } else {
            id = i->second;
        }
        logfile << "<inst_id>" << id << "</inst_id>" << endl;
    }

    logfile << "</message>" << endl;
}

string FPLog::getStackTrace(unsigned skipFrames)
{
    stringstream ss;
    void *ptr;
    ss.clear();
    ss.str("");

    if (stwalk_enabled) {
        stackwalk.clear();
        walker->walkStack(stackwalk); 
        for (unsigned i=0; i < stackwalk.size(); i++) { 
            // skip first few frames
            if (i >= skipFrames) {
                stackwalk[i].getName(stwalk_func); 
                stackwalk[i].getLibOffset(stwalk_lib, stwalk_offset, ptr); 
                ss << "<frame level=\"" << i-skipFrames << "\" address=\"0x";
                if (stwalk_func == "") {
                    Function *func;
                    debug_symtab->getContainingFunction((Offset)stwalk_offset, func);
                    if (func) {
                        vector<string> names = func->getAllPrettyNames();
                        if (names.size() > 0) {
                            ss << " function=\"" << sanitize(names[0]) << "\"";
                        }
                    }
                } else {
                    ss << hex << stwalk_offset << dec << "\" function=\"" << stwalk_func << "\"";
                }
                stwalk_symtab = (Symtab*)ptr;
                stwalk_lines.clear();
                stwalk_symtab->getSourceLines(stwalk_lines, stwalk_offset);
                if (stwalk_lines.size() > 0) {
                    ss << " file=\"" << stwalk_lines[0]->getFile() << "\" lineno=\"" << stwalk_lines[0]->getLine() << "\"";
                }
                //Module *mod;
                //debug_symtab->findModuleByOffset(mod, stwalk_offset);
                //if (mod) {
                    //ss << " module=\"" << mod->fileName() << "\"";
                //}
                ss << " />" << endl;
            }
        }
    }

    return ss.str();
}

string FPLog::getFakeStackTrace(void *addr)
{
    stringstream ss;
    ss.clear();
    ss.str("");

    ss << "<frame level=\"0\" address=\"" << hex << addr << dec << "\"";
    if (debug_symtab) {
        stwalk_lines.clear();
        debug_symtab->getSourceLines(stwalk_lines, (Offset)addr);
        /* 
         * NOTE: this causes Symtab to re-sort all functions; do not call often
         * while instrumenting!
         *
         *Function *func;
         *debug_symtab->getContainingFunction((Offset)addr, func);
         *if (func) {
         *    vector<string> names = func->getAllPrettyNames();
         *    if (names.size() > 0) {
         *       ss << " function=\"" << sanitize(names[0]) << "\"";
         *    }
         *}
         */
        if (stwalk_lines.size() > 0) {
            ss << " file=\"" << stwalk_lines[0]->getFile() << "\" lineno=\"" << stwalk_lines[0]->getLine() << "\"";
            /*
             * NOTE: more Symtab overhead...
             *
             *Module *mod;
             *debug_symtab->findModuleByOffset(mod, (Offset)addr);
             *if (mod) {
             *    ss << " module=\"" << mod->fileName() << "\"";
             *}
             */
        }
    }
    ss << " />" << endl;

    return ss.str();
}

string FPLog::getSourceLineInfo(void *addr)
{
    stringstream ss;
    ss.clear();
    ss.str("");
    if (debug_symtab) {
        stwalk_lines.clear();
        debug_symtab->getSourceLines(stwalk_lines, (Offset)addr);
        if (stwalk_lines.size() > 0) {
            ss << stwalk_lines[0]->getFile() << ":" << stwalk_lines[0]->getLine();
        }
    }
    return ss.str();
}

string FPLog::getSourceFunction(void *addr)
{
    stringstream ss;
    ss.clear();
    ss.str("");
    if (debug_symtab) {
        Function *func;
        debug_symtab->getContainingFunction((Offset)addr, func);
        if (func) {
            vector<string> names = func->getAllPrettyNames();
            if (names.size() > 0) {
                ss << sanitize(names[0]);
            }
        }
    }
    return ss.str();
}

void FPLog::getGlobalVars(vector<Variable *> &gvars)
{
    vector<Variable *> vars;
    vector<Symbol *> syms;
    vector<Variable *>::iterator i;
    vector<Symbol *>::iterator j;
    Variable *var;
    Symbol *sym;
    bool global;
    if (debug_symtab) {
        debug_symtab->getAllVariables(vars);
        for (i = vars.begin(); i != vars.end(); i++) {
            var = *i;
            syms.clear();
            var->getSymbols(syms);
            global = false;
            for (j = syms.begin(); j != syms.end(); j++) {
                sym = *j;
                if (sym->getLinkage() == Symbol::SL_GLOBAL) {
                    global = true;
                }
                if (global) {
                    gvars.push_back(var);
                }
            }
        }
    }
}

FPOperandAddress FPLog::getGlobalVarAddr(string name)
{
    vector<Variable *> vars;
    bool foundVars = false;
    FPOperandAddress ret = 0;
    if (debug_symtab) {
        foundVars = debug_symtab->findVariablesByName(vars, name);
        if (foundVars && vars.size() > 0) {
            ret = (FPOperandAddress)vars[0]->getOffset();
        }
    }
    return ret;
}

string FPLog::getGlobalVarName(FPOperandAddress addr)
{
vector<Variable *> vars;
vector<Variable *>::iterator i;
string ret("");
if (debug_symtab) {
	debug_symtab->getAllVariables(vars);
	for (i=vars.begin(); i!=vars.end(); i++) {
		if ((*i)->getOffset() == (Offset)addr) {
			vector<string> names = (*i)->getAllPrettyNames();
			if (names.size() > 0) {
				ret = names[0];
			}
		}
	}
}

/*
    Variable *var;
    bool foundVar = false;
    string ret("");
    if (debug_symtab) {
        foundVar = debug_symtab->findVariableByOffset(var, (Offset)addr);
        if (foundVar && var) {
            vector<string> names = var->getAllPrettyNames();
            if (names.size() > 0) {
                ret = names[0];
            }
        }
    }
*/
    return ret;
}

/*
unsigned long FPLog::getGlobalVarSize(string name)
    vector<Variable *> vars;
    bool foundVars = false;
    unsigned long ret = 0;
    Type *t, *bt;
    dataClass dclass;
    typeArray *ta;
    if (debug_symtab) {
        foundVars = debug_symtab->findVariablesByName(vars, name);
        if (foundVars && vars.size() > 0) {
            t = vars[0]->getType();
        }
    }
    if (t) {
        dclass = t->getDataClass();
        ret = t->getSize();
        //cout << "data class: " << dclass << endl;
        if (dclass == dataArray) {
            //cout << " **** array type!" << endl;
            ta = t->getArrayType();
            if (ta) {
                bt = ta->getBaseType();
                if (bt) {
                    ret = bt->getSize();
                    //cout << bt->getName() << " " << ret << endl;
                }
            }
        }
    }
    cout << "global var size: " << ret << endl;
}
*/

unsigned FPLog::getGlobalVarSize(string name)
{
    unsigned ret = 0;
    Type *t = getGlobalVarType(name);
    dataClass dclass = t->getDataClass();
    while (t && dclass != dataScalar) {
        ret = t->getSize();
        //cout << "data class: " << dclass << endl;
        if (dclass == dataArray) {
            //cout << " **** array type!" << endl;
            typeArray *ta = t->getArrayType();
            if (ta) {
                t = ta->getBaseType();
            }
        } else if (dclass == dataPointer) {
            //cout << " **** pointer type!" << endl;
            typePointer *tp = t->getPointerType();
            if (tp) {
                t = tp->getConstituentType();
            }
        }
        dclass = t->getDataClass();
    }
    //cout << "global var size: " << ret << endl;
    /*
    Variable *var;
    bool foundVar = false;
    if (debug_symtab) {
        foundVar = debug_symtab->findVariableByOffset(var, (Offset)addr);
        if (foundVar && var) {
            ret = var->getSize();
        }
    }
    */
    return ret;
}

unsigned FPLog::getGlobalVarSize(FPOperandAddress addr)
{
    unsigned ret = 0;
    Type *t = getGlobalVarType(addr);
    dataClass dclass = t->getDataClass();
    while (t && dclass != dataScalar) {
        ret = t->getSize();
        //cout << "data class: " << dclass << " size=" << ret << endl;
        if (dclass == dataArray) {
            //cout << " **** array type!" << endl;
            typeArray *ta = t->getArrayType();
            if (ta) {
                t = ta->getBaseType();
            }
        } else if (dclass == dataPointer) {
            //cout << " **** pointer type!" << endl;
            typePointer *tp = t->getPointerType();
            if (tp) {
                t = tp->getConstituentType();
            }
        } else if (dclass == dataTypedef) {
            //cout << " **** typedef type!" << endl;
            typeTypedef *tt = t->getTypedefType();
            if (tt) {
                t = tt->getConstituentType();
            }
        } else if (dclass != dataScalar) {
            cerr << "Invalid data type: " << dclass << endl;
            break;
        }
        dclass = t->getDataClass();
    }
    //cout << "global var size: " << ret << endl;
    /*
    Variable *var;
    bool foundVar = false;
    if (debug_symtab) {
        foundVar = debug_symtab->findVariableByOffset(var, (Offset)addr);
        if (foundVar && var) {
            ret = var->getSize();
        }
    }
    */
    return ret;
}

Type* FPLog::getGlobalVarType(string name)
{
    vector<Variable *> vars;
    bool foundVars = false;
    Type* ret = NULL;
    if (debug_symtab) {
        foundVars = debug_symtab->findVariablesByName(vars, name);
        if (foundVars && vars.size() > 0) {
            ret = vars[0]->getType();
        }
    }
    return ret;
}


Type* FPLog::getGlobalVarType(FPOperandAddress addr)
{
    Variable *var;
    bool foundVar = false;
    Type* ret = NULL;
    if (debug_symtab) {
        foundVar = debug_symtab->findVariableByOffset(var, (Offset)addr);
        if (foundVar && var) {
            ret = var->getType();
        }
    }
    return ret;
}

string FPLog::sanitize(const string &text)
{
    string cleaned = text;
    string::size_type pos = 0;
    for (pos = 0; pos < cleaned.size(); pos++) {
        if (cleaned[pos] == '<')
            cleaned[pos] = '{';
        if (cleaned[pos] == '>')
            cleaned[pos] = '}';
        if (cleaned[pos] == '&')
            cleaned[pos] = '`';
    }
    /*
    while (pos = cleaned.find("<") != string::npos) {
        cleaned.replace(pos, 1, "&lt;");
    }
    while (pos = cleaned.find(">") != string::npos) {
        cleaned.replace(pos, 1, "&gt;");
    }
    while (pos = cleaned.find("&") != string::npos) {
        cleaned.replace(pos, 1, "&amp;");
    }
    while (pos = cleaned.find("\"") != string::npos) {
        cleaned.replace(pos, 1, "&quot;");
    }
    while (pos = cleaned.find("'") != string::npos) {
        cleaned.replace(pos, 1, "&apos;");
    }
    */
    return cleaned;
}

void FPLog::writeTraces()
{
    map<string, unsigned>::iterator i;
    for (i = traces.begin(); i != traces.end(); i++) {
        logfile << "<trace id=\"" << (i->second) << "\">" << endl;
        logfile << (i->first) << "</trace>" << endl;
    }
}

void FPLog::writeInstructions()
{
    map<FPSemantics *, unsigned>::iterator i;
    for (i = instructions.begin(); i != instructions.end(); i++) {
        logfile << "<instruction id=\"" << (i->second) << "\"";
        logfile << " address=\"" << hex << i->first->getAddress() << dec << "\"";
        if (debug_symtab) {
            stwalk_lines.clear();
            debug_symtab->getSourceLines(stwalk_lines, (Offset)i->first->getAddress());
            Function *func;
            debug_symtab->getContainingFunction((Offset)i->first->getAddress(), func);
            if (func) {
                vector<string> names = func->getAllPrettyNames();
                if (names.size() > 0) {
                    logfile << " function=\"" << sanitize(names[0]) << "\"";
                }
            }
            if (stwalk_lines.size() > 0) {
                logfile << " file=\"" << stwalk_lines[0]->getFile() << "\" lineno=\"" << stwalk_lines[0]->getLine() << "\"";
                //Module *mod;
                //debug_symtab->findModuleByOffset(mod, (Offset)i->first->getAddress());
                //if (mod) {
                    //logfile << " module=\"" << mod->fileName() << "\"" << endl;
                //}
            }
        }
        logfile << ">" << endl;
        logfile << "<disassembly>" << endl << (i->first->getDisassembly()) << endl << "</disassembly>" << endl;
        logfile << "<text>" << endl << (i->first->toString()) << endl << "</text>" << endl;
        logfile << "</instruction>" << endl;
    }
}

string FPLog::formatLargeCount(size_t val)
{
    stringstream ss;
    ss.clear(); ss.str("");
    int divs = 0, dec = 0;
    while (val > 10000) {
        val /= 1000;
        divs++;
    }
    if (val > 1000) {
        val = val / 100;
        dec = val % 10;
        val = val / 10;
        divs++;
    }
    ss << val << "." << dec;
    switch (divs) {
        case 0:  break;
        case 1:  ss << "K"; break;
        case 2:  ss << "M"; break;
        case 3:  ss << "G"; break;
        case 4:  ss << "T"; break;
        case 5:  ss << "P"; break;
        case 6:  ss << "E"; break;
        default: ss << "e" << divs*3; break;
    }
    return ss.str();
}

void FPLog::close()
{
    if (!fileOpen) return;
    writeTraces();
    writeInstructions();
    logfile << "</log>" << endl;
    logfile.close();
    fileOpen = false;
}

}

