/**
 * OpenConfigAction
 *
 * display dialog and start opening a configuration
 */

import java.awt.event.*;
import javax.swing.*;

public class OpenConfigAction extends AbstractAction {

    ConfigEditorApp appInst;

    public OpenConfigAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        int returnVal;
        returnVal = appInst.fileChooser.showOpenDialog(appInst);

        if (returnVal == JFileChooser.APPROVE_OPTION) {
            appInst.openFile(appInst.fileChooser.getSelectedFile());
        }
    }
}

