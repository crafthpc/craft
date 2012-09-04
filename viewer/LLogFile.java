/**
 * LLogFile
 *
 * represents a logfile and contains collections of messages, stack 
 * traces, and instructions
 */

import java.io.*;
import java.util.*;

public class LLogFile {

    public java.util.List<LMessage> messages;
    public Map<String, LTrace> traces;
    public Map<String, LInstruction> instructions;
    public Map<String, LInstruction> instructionsByAddress;
    public String appname;
    public File file;

    public LLogFile() {
        messages = new ArrayList<LMessage>();
        traces = new HashMap<String, LTrace>();
        instructions = new HashMap<String, LInstruction>();
        instructionsByAddress = new HashMap<String, LInstruction>();
        appname = "";
        file = null;
    }

    public void merge(LLogFile logfile) {
        Map<String, String> traceMapping = new HashMap<String, String>();
        Map<String, String> instrMapping = new HashMap<String, String>();
        boolean found;
        for (LMessage msg : logfile.messages) {
            if (!(msg.backtraceID.equals(""))) {
                String oldTraceID = msg.backtraceID;
                LTrace oldTrace = logfile.traces.get(oldTraceID);
                if (traceMapping.containsKey(msg.backtraceID)) {
                    msg.backtraceID = traceMapping.get(oldTraceID);
                    msg.backtrace = traces.get(msg.backtraceID);
                } else {
                    for (LTrace trace : traces.values()) {
                        if (trace.equals(oldTrace)) {
                            traceMapping.put(msg.backtraceID, trace.id);
                            msg.backtraceID = trace.id;
                            msg.backtrace = trace;
                            break;
                        }
                    }
                    if (msg.backtraceID.equals(oldTraceID)) {
                        String newTraceID = (new Integer(traces.size()+1)).toString();
                        oldTrace.id = newTraceID;
                        traces.put(newTraceID, oldTrace);
                        traceMapping.put(oldTraceID, newTraceID);
                        msg.backtraceID = newTraceID;
                    }
                }
            }
            if (!(msg.instructionID.equals(""))) {
                String oldInstrID = msg.instructionID;
                LInstruction oldInstr = logfile.instructions.get(oldInstrID);
                if (instrMapping.containsKey(msg.instructionID)) {
                    msg.instructionID = instrMapping.get(oldInstrID);
                    msg.instruction = instructions.get(msg.instructionID);
                } else {
                    for (LInstruction instr : instructions.values()) {
                        if (instr.address.equals(oldInstr.address)) {
                            instrMapping.put(msg.instructionID, instr.id);
                            msg.instructionID = instr.id;
                            msg.instruction = instr;
                            break;
                        }
                    }
                    if (msg.instructionID.equals(oldInstrID)) {
                        String newInstrID = (new Integer(instructions.size()+1)).toString();
                        oldInstr.id = newInstrID;
                        instructions.put(newInstrID, oldInstr);
                        // no need to re-insert into instructionsByAddress
                        instrMapping.put(oldInstrID, newInstrID);
                        msg.instructionID = newInstrID;
                    }
                }
            }
            messages.add(msg);
        }
        for (LInstruction instr : logfile.instructions.values()) {
            found = false;
            for (LInstruction i : instructions.values()) {
                if (instr.address.equals(i.address)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                String newInstrID = (new Integer(instructions.size()+1)).toString();
                instr.id = newInstrID;
                instructions.put(instr.id, instr);
                instructionsByAddress.put(instr.address, instr);
            }
        }
    }

    public String toString() {
        String str = "Log for " + appname + "\n";
        for (LMessage msg : messages) {
            str += msg.toString() + "\n";
        }
        /*
        for (String i : traces.keySet()) {
            str += "Backtrace " + i.toString() + ":";
            for (LFrame frame : traces.get(i)) {
                str += "\n   " + frame.toString();
            }
        }
        */
        /*
        for (String i : instructions.keySet()) {
            str += "Instruction " + i.toString() + ":";
            str += "\n" + instructions.get(i).toString();
        }
        */
        return str;
    }

}

