/**
 * ReportAction
 *
 * execute and display report
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class ReportAction extends AbstractAction {

    ViewerApp appInst;
    Report reportInst;

    public ReportAction(ViewerApp app, Report report,
            String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        reportInst = report;
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        reportInst.runReport(appInst.mainLogFile);

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(reportInst.generateUI(), BorderLayout.CENTER);
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10,5,5,5));

        JDialog reportDialog = new JDialog(appInst, reportInst.getTitle(), false);
        reportDialog.getContentPane().add(mainPanel);
        reportDialog.setSize(600, 800);
        reportDialog.setLocationRelativeTo(null);
        reportDialog.setVisible(true);
    }

}

