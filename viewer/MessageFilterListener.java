/**
 * MessageFilterListener
 *
 * filters messages by type
 */

import java.awt.event.*;
import javax.swing.*;

public class MessageFilterListener implements ActionListener {

    private JTable messageTable;
    private JLabel messageLabel;

    public MessageFilterListener(JTable table, JLabel label) {
        messageTable = table;
        messageLabel = label;
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
            //messageLabel.setText(model.getSize() + " message(s):");
        }
    }

}

