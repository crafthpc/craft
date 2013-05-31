/**
 * RemoveNonCandidateAction
 *
 * call appropriate method on app instance
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class RemoveNonCandidateAction extends AbstractAction implements ActionListener {

    ConfigEditorApp appInst;

    public RemoveNonCandidateAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        appInst.removeNonCandidateEntries();
    }
}

