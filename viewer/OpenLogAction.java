/**
 * OpenLogAction
 *
 * display dialog and start loading a logfile
 */

import java.awt.event.*;
import javax.swing.*;

public class OpenLogAction extends AbstractAction {

    ViewerApp appInst;

    public OpenLogAction(ViewerApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        int returnVal;
        appInst.fileChooser.setFileFilter(new LogFilter());
        returnVal = appInst.fileChooser.showOpenDialog(appInst);

        if (returnVal == JFileChooser.APPROVE_OPTION) {
            appInst.openLogFile(appInst.fileChooser.getSelectedFile());
        }
    }

}

