/**
 * LInstruction
 *
 * holds detailed information about an instruction
 */

public class LInstruction {

    public String id;
    public String address;
    public String text;
    public String disassembly;
    public String function;
    public String file;
    public String lineno;
    public String module;
    public long count;
    public long cancellations;
    public long totalCancels;
    public double ratio;
    public double averageDigits;
    public double min;
    public double max;
    public double range;
    public String rstatus;  // replacement status

    public LInstruction() {
        id = ""; address = ""; text = ""; disassembly = "";
        function = ""; file = ""; lineno = ""; module = "";
        count = 0; cancellations = 0; totalCancels = 0;
        ratio = 0.0; averageDigits = 0.0;
        min = Double.POSITIVE_INFINITY;
        max = Double.NEGATIVE_INFINITY;
        range = Double.POSITIVE_INFINITY;
        rstatus = "";
    }

    public void updateRatio() {
        if (count != 0 && totalCancels != 0) {
            ratio = (double)totalCancels / (double)count;
        } else if (count != 0) {
            ratio = (double)cancellations / (double)count;
        } else {
            ratio = 0.0;
        }
    }

    public String getSource() {
        if (!(file.equals("")) && !(lineno.equals("")))
            return file + ":" + lineno;
        else
            return "";
    }

    public String toString() {
        return text;
    }

}

