/**
 * InstructionAssemblyReport
 */

import java.awt.*;
import java.awt.event.*;
import java.math.*;
import java.util.*;
import javax.swing.*;

public class InstructionAssemblyReport implements Report {

    private Map<String, BigInteger> typeCounts;
    private String resultText;

    public void runReport(LLogFile log) {
        typeCounts = new TreeMap<String, BigInteger>();
        resultText = "";
        if (log == null || log.messages == null) return;
        for (LMessage msg : log.messages) {
            if (msg.type.equals("InstCount")) {
                LInstruction inst = msg.instruction;
                String name = inst.disassembly;
                if (name != null) {
                    name = name.trim();
                    if (typeCounts.containsKey(name)) {
                        typeCounts.put(name, typeCounts.get(name).add(new BigInteger(msg.priority)));
                    } else {
                        typeCounts.put(name, new BigInteger(msg.priority));
                    }
                }
            }
        }
        StringBuffer buffer = new StringBuffer();
        for (Map.Entry<String, BigInteger> entry : typeCounts.entrySet()) {
            buffer.append(entry.getValue().toString());
            buffer.append("\t");
            buffer.append(entry.getKey());
            buffer.append("\n");
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
        return "Instructions by Assembly Report (Runtime)";
    }

    public String getText() {
        return resultText;
    }

}

