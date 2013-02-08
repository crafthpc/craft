/**
 * ExpandRowsAction
 *
 * call appropriate method on app instance
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class ExpandRowsAction extends AbstractAction implements ActionListener {

    ConfigEditorApp appInst;
    String tag;

    public ExpandRowsAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic, String tag) {
        super(text, icon);
        this.appInst = app;
        this.tag = tag;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        if (tag.equals("all")) {
            appInst.expandAllRows();
        } else if (tag.equals("none")) {
            appInst.collapseAllRows();
        } else if (tag.equals("single")) {
            appInst.expandRows(ConfigTreeNode.CNStatus.SINGLE);
        } else if (tag.equals("double")) {
            appInst.expandRows(ConfigTreeNode.CNStatus.DOUBLE);
        } else if (tag.equals("tested")) {
            appInst.expandTestedRows();
        }
    }
}

