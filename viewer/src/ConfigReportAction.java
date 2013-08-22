/**
 * ConfigReportAction
 *
 * execute and display report
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class ConfigReportAction extends AbstractAction {

    ConfigEditorApp appInst;
    ConfigReport reportInst;

    public ConfigReportAction(ConfigEditorApp app, ConfigReport report,
            String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        reportInst = report;
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        reportInst.runReport(appInst.getRootNode());

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(reportInst.generateUI(), BorderLayout.CENTER);
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10,5,5,5));

        JDialog reportDialog = new JDialog(appInst, reportInst.getTitle(), false);
        reportDialog.getContentPane().add(mainPanel);
        reportDialog.setSize(900, 600);
        reportDialog.setLocationRelativeTo(null);
        reportDialog.setVisible(true);
    }

}

