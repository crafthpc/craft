/**
 * InstructionTypeReport
 */

import java.awt.*;
import java.awt.event.*;
import java.math.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.*;

public class InstructionTypeReport implements Report {

    private String resultText;

    public void runReport(LLogFile log) {
        // analysis type => { mnemonic => count }
        Map<String, Map<String, BigInteger>> instrumentedTypeCounts = new TreeMap<String, Map<String, BigInteger>>();
        // analysis type => total_count
        Map<String, BigInteger> totalInstrumentedCounts = new TreeMap<String, BigInteger>();
        // mnemonic => count
        Map<String, BigInteger> ignoredTypeCounts = new TreeMap<String, BigInteger>();
        BigInteger totalIgnored = BigInteger.ZERO;
        String instPatternStr = "^Inserted ([\\w ]*) ([\\w-]*)instrumentation.*";
        Pattern instPattern = Pattern.compile(instPatternStr);
        resultText = "";
        if (log == null || log.messages == null) return;
        for (LMessage msg : log.messages) {
            LInstruction inst = msg.instruction;
            if (inst != null) {
                String[] parts = inst.disassembly.trim().split("\\ ");
                if (parts != null && parts.length > 0) {
                    String mnemonic = parts[0];
                    if (msg.type.equals("Status") && msg.label.matches(instPatternStr)) {
                        Matcher m = instPattern.matcher(msg.label);
                        if (m.find()) {
                            String itype = m.group(1);
                            if (!instrumentedTypeCounts.containsKey(itype)) {
                                instrumentedTypeCounts.put(itype, new TreeMap<String, BigInteger>());
                                totalInstrumentedCounts.put(itype, BigInteger.ZERO);
                            }
                            Map<String, BigInteger> currTypeCount = instrumentedTypeCounts.get(itype);
                            if (currTypeCount.containsKey(mnemonic)) {
                                currTypeCount.put(mnemonic, currTypeCount.get(mnemonic).add(BigInteger.ONE));
                            } else {
                                currTypeCount.put(mnemonic, BigInteger.ONE);
                            }
                            totalInstrumentedCounts.put(itype, 
                                    totalInstrumentedCounts.get(itype).add(BigInteger.ONE));
                        }
                    } else if (msg.type.equals("Warning") && msg.label.matches("^Ignored instruction.*")) {
                        if (ignoredTypeCounts.containsKey(mnemonic)) {
                            ignoredTypeCounts.put(mnemonic, ignoredTypeCounts.get(mnemonic).add(BigInteger.ONE));
                        } else {
                            ignoredTypeCounts.put(mnemonic, BigInteger.ONE);
                        }
                        totalIgnored = totalIgnored.add(BigInteger.ONE);
                    }
                }
            }
        }
        StringBuffer buffer = new StringBuffer();
        for (Map.Entry<String, Map<String, BigInteger>> ientry : instrumentedTypeCounts.entrySet()) {
            String itype = ientry.getKey();
            Map<String, BigInteger> itypeCounts = ientry.getValue();
            buffer.append("\nInstrumented (");
            buffer.append(itype);
            buffer.append("):  [" + totalInstrumentedCounts.get(itype).toString() + " total]\n");
            for (Map.Entry<String, BigInteger> entry : itypeCounts.entrySet()) {
                buffer.append(entry.getValue().toString());
                buffer.append("\t");
                buffer.append(entry.getKey());
                buffer.append("\n");
            }
        }
        if (totalIgnored.compareTo(BigInteger.ZERO) > 0) {
            buffer.append("\nIgnored:  [" + totalIgnored.toString() + " total]\n");
            for (Map.Entry<String, BigInteger> entry : ignoredTypeCounts.entrySet()) {
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
        return "Instructions by Type Report";
    }

    public String getText() {
        return resultText;
    }

}

