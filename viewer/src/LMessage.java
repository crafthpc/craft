/**
 * LMessage
 *
 * represents a single message/event in a logfile
 * contains pointers to stack traces and instruction info
 */

public class LMessage {

    public String time;
    public String priority;
    public String type;
    public String label;
    public String details;
    public String backtraceID;
    public String instructionID;
    public LTrace backtrace;
    public LInstruction instruction;

    public LMessage() {
        time = ""; priority = "";
        type = ""; label = ""; details = "";
        backtraceID = ""; backtrace = null;
        instructionID = ""; instruction = null;
    }

    public String getAddress() {
        if (instruction == null)
            return "";
        else
            return instruction.address;
    }

    public String getFunction() {
        if (backtrace == null || backtrace.getSize() == 0) {
            if (instruction != null && instruction.function != null) {
                return instruction.function;
            } else {
                return "";
            }
        }
        else {
            return backtrace.frames.get(0).function;
        }
    }

    public String toString() {
        int p;
        if (priority.equals("ALL")) {
            p = 999;
        } else {
            p = Integer.parseInt(priority);
        }
        return (p == 999 ? "[***] " : (p > 20 ? "[**] " : (p > 0 ? "[*] " : "")))
            + type + ": " + label
            + (details.equals("") ? "" : "\n  " + details)
            + (backtrace == null ? "" : "\n  Backtrace:" + backtrace.toString())
            + (instruction == null ? "" : "\n  Instruction:\n" + instruction.toString());
    }

}

