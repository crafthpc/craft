/**
 * MessageFilterModel
 *
 * handles message counts for filter combo box
 */

import javax.swing.*;

public class MessageFilterModel extends DefaultComboBoxModel {

    public final String[] MESSAGE_FILTERS = {
        "(none)", "(address)", "Status", "Error", "Warning", 
        "Summary", "Cancellation", "InstCount", "ShadowVal" };

    public LLogFile logfile;

    public MessageFilterModel() {
        this(null);
    }

    public MessageFilterModel(LLogFile log) {
        logfile = log;
        refreshData();
    }

    public void setLogFile(LLogFile log) {
        logfile = log;
        refreshData();
    }

    void refreshData() {
        int[] counts = new int[MESSAGE_FILTERS.length];
        int i, selected;
        for (i = 0; i < counts.length; i++) {
            counts[i] = 0;
        }
        if (logfile != null) {
            for (LMessage msg : logfile.messages) {
                for (i = 0; i < MESSAGE_FILTERS.length; i++) {
                    if (msg.type.equals(MESSAGE_FILTERS[i])) {
                        counts[i] += 1;
                    }
                }
            }
        }
        if (getSelectedItem() != null) {
            selected = getIndexOf(getSelectedItem());
        } else {
            selected = 0;
        }
        removeAllElements();
        for (i = 0; i < counts.length; i++) {
            String item = MESSAGE_FILTERS[i];
            if (i > 0 && counts[i] > 0) {
                item += " (" + counts[i] + ")";
            }
            addElement(item);
        }
        setSelectedItem(getElementAt(selected));
    }

}

