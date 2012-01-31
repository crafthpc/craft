/**
 * CancellationAction
 *
 * report statistics on cancellations
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class CancellationReport implements Report {

    private String resultText;

    public void runReport(LLogFile log) {
        StringBuffer report = new StringBuffer();
        int cancels, totalCancellations = 0, totalOperations = 0;
        double avgDigits = 0.0;
        resultText = "";
        if (log == null || log.instructions == null) return;
        for (LInstruction inst : log.instructions.values()) {
            if (inst.disassembly.indexOf("add") >= 0 ||
                inst.disassembly.indexOf("sub") >= 0
                    ) {
                totalOperations += inst.count;
            }
            cancels = Math.max(inst.cancellations, inst.totalCancels);
            totalCancellations += cancels;
            avgDigits += inst.averageDigits * (double)cancels;
        }
        avgDigits /= (double)totalCancellations;
        report.append("Total add/sub operations: ");
        report.append(totalOperations);
        report.append("\n");
        report.append("Total cancellations: ");
        report.append(totalCancellations);
        report.append("\n");
        report.append("Average digits: ");
        report.append(avgDigits);
        resultText = report.toString();
    }

    public Component generateUI() {
        JTextArea textbox = new JTextArea();
        textbox.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        textbox.setText(resultText);
        textbox.setEditable(false);
        return new JScrollPane(textbox);
    }

    public String getTitle() {
        return "Cancellation Report";
    }

    public String getText() {
        return resultText;
    }

}

