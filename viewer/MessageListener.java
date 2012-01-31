/**
 * MessageListener
 *
 * loads detailed info (including stack trace) when a message is selected
 */

import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class MessageListener extends MouseAdapter implements ListSelectionListener {

    private JPanel topPanel;
    private JTable messageList;
    private JTextArea detailsLabel;
    private JList backtrace;

    private final DefaultListModel BLANK_LIST = new DefaultListModel();

    public MessageListener(JPanel panel, JTable log, JTextArea details, JList trace) {
        topPanel = panel;
        messageList = log;
        detailsLabel = details;
        backtrace = trace;
    }

    public void valueChanged(ListSelectionEvent e) {
        ListSelectionModel lsm = (ListSelectionModel)e.getSource();
        if (messageList.getModel() instanceof MessageModel) {
            MessageModel model = (MessageModel)messageList.getModel();
            if (lsm.isSelectionEmpty() || model == null) {
                backtrace.setModel(BLANK_LIST);
            } else {
                LMessage msg = model.messages.get(lsm.getMinSelectionIndex());
                detailsLabel.setText(msg.details +
                        (msg.instruction == null ? "" : 
                         System.getProperty("line.separator") + msg.instruction.toString()));
                if (msg.backtrace != null) {
                    backtrace.setModel(msg.backtrace);
                    if (msg.backtrace.getSize() > 0)
                        backtrace.setSelectedIndex(0);
                } else if (msg.instruction != null) {
                    // construct fake backtrace
                    LFrame frame = new LFrame();
                    frame.level = "0";
                    frame.address = msg.instruction.address;
                    frame.function = msg.instruction.function;
                    frame.file = msg.instruction.file;
                    frame.lineno = msg.instruction.lineno;
                    frame.module = msg.instruction.module;
                    LTrace trace = new LTrace();
                    trace.frames.add(frame);
                    backtrace.setModel(trace);
                    if (trace.getSize() > 0)
                        backtrace.setSelectedIndex(0);
                }
                else {
                    backtrace.setModel(BLANK_LIST);
                }
            }
        } else {
            backtrace.setModel(BLANK_LIST);
        }
        if (topPanel != null) {
            topPanel.revalidate();
        }
    }

}

