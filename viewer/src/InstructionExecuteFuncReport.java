/**
 * InstructionExecuteFuncReport
 */

import java.awt.*;
import java.awt.event.*;
import java.math.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.*;

public class InstructionExecuteFuncReport implements Report {

    private String resultText;

    public void runReport(LLogFile log) {
        // filename => { function => count }
        Map<String, Map<String, BigInteger>> instrumentedFuncCounts = new TreeMap<String, Map<String, BigInteger>>();
        // filename => total_count
        Map<String, BigInteger> totalModuleCounts = new TreeMap<String, BigInteger>();
        resultText = "";
        if (log == null || log.messages == null) return;
        for (LMessage msg : log.messages) {
            if (msg.type.equals("InstCount")) {
                LInstruction inst = msg.instruction;
                if (inst != null) {
                    String filename = inst.file;
                    String function = inst.function;
                    BigInteger count = new BigInteger(msg.priority);
                    if (!totalModuleCounts.containsKey(filename)) {
                        instrumentedFuncCounts.put(filename, new TreeMap<String, BigInteger>());
                        totalModuleCounts.put(filename, BigInteger.ZERO);
                    }
                    Map<String, BigInteger> currFuncCount = instrumentedFuncCounts.get(filename);
                    if (currFuncCount.containsKey(function)) {
                        currFuncCount.put(function, currFuncCount.get(function).add(count));
                    } else {
                        currFuncCount.put(function, count);
                    }
                    totalModuleCounts.put(filename, 
                            totalModuleCounts.get(filename).add(count));
                }
            }
        }
        StringBuffer buffer = new StringBuffer();
        for (Map.Entry<String, Map<String, BigInteger>> ientry : instrumentedFuncCounts.entrySet()) {
            String filename = ientry.getKey();
            Map<String, BigInteger> filenameCounts = ientry.getValue();
            buffer.append("\nExecuted from ");
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
        return "Instructions by Function Report (Runtime)";
    }

    public String getText() {
        return resultText;
    }
}

