/**
 * InstructionFuncReport
 */

import java.awt.*;
import java.awt.event.*;
import java.math.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.*;

public class InstructionFuncReport implements Report {

    private Map<String, Integer> funcCounts;
    private String resultText;

    public void runReport(LLogFile log) {
        // filename => { function => count }
        Map<String, Map<String, BigInteger>> instrumentedFuncCounts = new TreeMap<String, Map<String, BigInteger>>();
        // filename => total_count
        Map<String, BigInteger> totalModuleCounts = new TreeMap<String, BigInteger>();
        String instPatternStr = "^Inserted ([\\w ]*) instrumentation.*";
        resultText = "";
        if (log == null || log.messages == null) return;
        for (LMessage msg : log.messages) {
            if (msg.label.matches(instPatternStr)) {
                LInstruction inst = msg.instruction;
                if (inst != null) {
                    String filename = inst.file;
                    String function = inst.function;
                    if (!totalModuleCounts.containsKey(filename)) {
                        instrumentedFuncCounts.put(filename, new TreeMap<String, BigInteger>());
                        totalModuleCounts.put(filename, BigInteger.ZERO);
                    }
                    Map<String, BigInteger> currFuncCount = instrumentedFuncCounts.get(filename);
                    if (currFuncCount.containsKey(function)) {
                        currFuncCount.put(function, currFuncCount.get(function).add(BigInteger.ONE));
                    } else {
                        currFuncCount.put(function, BigInteger.ONE);
                    }
                    totalModuleCounts.put(filename, 
                            totalModuleCounts.get(filename).add(BigInteger.ONE));
                }
            }
        }
        StringBuffer buffer = new StringBuffer();
        for (String filename : instrumentedFuncCounts.keySet()) {
            Map<String, BigInteger> filenameCounts = instrumentedFuncCounts.get(filename);
            buffer.append("\nInstrumented ");
            buffer.append(filename);
            buffer.append(":  [" + totalModuleCounts.get(filename).toString() + " total]\n");
            for (Map.Entry<String, BigInteger> entry : filenameCounts.entrySet()) {
                buffer.append(entry.getValue().toString());
                buffer.append("\t");
                buffer.append(entry.getKey());
                buffer.append("\n");
            }
        }
        resultText = buffer.toString();
    }

    public Component generateUI() {
        JTextArea textbox = new JTextArea();
        textbox.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        textbox.setText(resultText);
        textbox.setEditable(false);
        return new JScrollPane(textbox);
    }

    public String getTitle() {
        return "Instructions by Function Report";
    }

    public String getText() {
        return resultText;
    }
}

