/**
 * MessageFilterListener
 *
 * filters messages by type
 */

import java.awt.event.*;
import javax.swing.*;

public class MessageFilterListener implements ActionListener {

    private JTable messageTable;

    public MessageFilterListener(JTable table) {
        messageTable = table;
    }

    public void actionPerformed(ActionEvent e) {
        JComboBox fbox = (JComboBox)e.getSource();
        String type = (String)fbox.getSelectedItem();
        if (type == null) {
            return;
        }
        if (type.equals("(none)")) {
            type = "";
        } else {
            String[] parts = type.split(" ");
            type = parts[0];
        }
        if (messageTable.getModel() instanceof MessageModel) {
            MessageModel model = (MessageModel)messageTable.getModel();
            model.setTypeFilter(type);
            model.refreshData();
            messageTable.revalidate();
        }
    }

}

