/**
 * LFrame
 *
 * holds detailed information about a stack trace frame
 */

public class LFrame {

    public String level;
    public String address;
    public String function;
    public String file;
    public String lineno;
    public String module;

    public LFrame() {
        level = ""; address = "";
        function = ""; file = ""; 
        lineno = ""; module = "";
    }

    public String toString() {
        return "#" + level + "  " + address + 
            (function.equals("") ? "" : " in " + function) +
            (file.equals("") ? "" : " at " + file + (lineno.equals("") ? "" : ":" + lineno)) +
            (module.equals("") ? "" : " from " + module);
    }

}

