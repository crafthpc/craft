/**
 * SummaryReport
 */

import java.awt.*;
import java.awt.event.*;
import java.math.*;
import java.util.*;
import javax.swing.*;

public class SummaryReport implements Report {

    private String resultText;

    public void runReport(LLogFile log) {
        StringBuffer buffer = new StringBuffer();
        if (log == null || log.messages == null) return;
        for (LMessage msg : log.messages) {
            if (msg.type.equals("Summary")) {
                buffer.append(msg.details);
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
        return "Summary Report";
    }

    public String getText() {
        return resultText;
    }

}

