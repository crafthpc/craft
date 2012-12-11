/**
 * RemoveMovementAction
 *
 * call appropriate method on app instance
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class RemoveMovementAction extends AbstractAction implements ActionListener {

    ConfigEditorApp appInst;

    public RemoveMovementAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        appInst.removeMovementEntries();
    }
}

