/**
 * BatchConfigAction
 *
 * display dialog and start opening a configuration
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class BatchConfigAction extends AbstractAction implements ActionListener {

    final ConfigTreeNode.CNStatus[] allStatuses = {
            ConfigTreeNode.CNStatus.CANDIDATE,
            ConfigTreeNode.CNStatus.SINGLE,
            ConfigTreeNode.CNStatus.DOUBLE,
            ConfigTreeNode.CNStatus.IGNORE,
            ConfigTreeNode.CNStatus.NONE };

    ConfigEditorApp appInst;

    JDialog batchConfigDialog;
    JComboBox origBox;
    JComboBox destBox;
    JButton okButton;
    JButton cancelButton;

    public BatchConfigAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void buildDialog() {
        JPanel origDestPanel = new JPanel();
        origBox = new JComboBox(allStatuses);
        destBox = new JComboBox(allStatuses);
        origDestPanel.add(new JLabel("Change from "));
        origDestPanel.add(origBox);
        origDestPanel.add(new JLabel(" to "));
        origDestPanel.add(destBox);

        JPanel buttonPanel = new JPanel();
        okButton = new JButton("OK");
        okButton.addActionListener(this);
        cancelButton = new JButton("Cancel");
        cancelButton.addActionListener(this);
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(origDestPanel, BorderLayout.CENTER);
        mainPanel.add(buttonPanel, BorderLayout.SOUTH);

        batchConfigDialog = new JDialog(appInst, "Batch Config",
                Dialog.ModalityType.APPLICATION_MODAL);
        batchConfigDialog.add(mainPanel);
        batchConfigDialog.setSize(400,100);
        batchConfigDialog.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        batchConfigDialog.setLocationRelativeTo(appInst);
    }

    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == okButton) {
            ConfigTreeNode.CNStatus origStatus, destStatus;
            origStatus = (ConfigTreeNode.CNStatus)origBox.getSelectedItem();
            destStatus = (ConfigTreeNode.CNStatus)destBox.getSelectedItem();
            appInst.batchConfig(origStatus, destStatus);
            batchConfigDialog.dispose();
        } else if (e.getSource() == cancelButton) {
            batchConfigDialog.dispose();
        } else { // menu action
            buildDialog();
            batchConfigDialog.setVisible(true);
        }
    }
}

