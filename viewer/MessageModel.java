/**
 * MessageModel
 *
 * handles table data for "Messages" tab
 */

import java.util.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;

public class MessageModel extends AbstractTableModel {

    public LLogFile logfile;
    public JLabel messageLabel;

    public java.util.List<LMessage> messages;
    public String typeFilter;
    public String addressFilter;

    public MessageModel(LLogFile log) {
        this(log, null);
    }

    public MessageModel(LLogFile log, JLabel label) {
        logfile = log;
        messageLabel = label;
        typeFilter = "";
        addressFilter = "";
        refreshData();
    }

    public void setTypeFilter(String type) {
        typeFilter = type;
        refreshData();
    }

    public void refreshData() {
        messages = new ArrayList<LMessage>();
        if (typeFilter.equals("")) {
            for (LMessage msg : logfile.messages) {
                messages.add(msg);
            }
        } else if (typeFilter.equals("(address)")) {
            for (LMessage msg : logfile.messages) {
                if (msg.instruction != null && msg.instruction.address.equals(addressFilter)) {
                    messages.add(msg);
                }
            }
        } else {
            for (LMessage msg : logfile.messages) {
                if (msg.type.equals(typeFilter)) {
                    messages.add(msg);
                }
            }
        }
        if (messageLabel != null) {
            messageLabel.setText(messages.size() + " message(s):");
        }
    }

    public Object getValueAt(int row, int col) {
        switch (col) {
            case 0: return messages.get(row).time;
            case 1: return messages.get(row).priority;
            case 2: return messages.get(row).type;
            case 3: return messages.get(row).getAddress();
            case 4: return messages.get(row).getFunction();
            case 5: return messages.get(row).label;
            default: return "";
        }
    }

    public int getSize() {
        return messages.size();
    }

    public int getColumnCount() {
        return 6;
    }

    public int getRowCount() {
        return messages.size();
    }

    public String getColumnName(int col) {
        switch (col) {
            case 0: return "Time";
            case 1: return "Digits";
            case 2: return "Type";
            case 3: return "Address";
            case 4: return "Function";
            case 5: return "Message";
            default: return "";
        }
    }

    public Class getColumnClass(int col) {
        return getValueAt(0,col).getClass();
    }

    public void setPreferredColumnSizes(TableColumnModel model) {
        model.getColumn(0).setPreferredWidth(50);
        model.getColumn(1).setPreferredWidth(50);
        model.getColumn(2).setPreferredWidth(100);
        model.getColumn(3).setPreferredWidth(100);
        model.getColumn(4).setPreferredWidth(150);
        model.getColumn(5).setPreferredWidth(250);
    }

    public boolean isCellEditable(int row, int col) {
        return false;
    }

    public void setValueAt(Object value, int row, int col) {
        return;
    }

    public void addTableModelListener(TableModelListener l) { }
    public void removeTableModelListener(TableModelListener l) { }

    public void sort(int column, boolean ascending) {
        Collections.sort(messages, new MessageComparator(column, ascending));
    }

}

