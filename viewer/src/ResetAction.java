/**
 * ResetAction
 *
 * close all logs
 */

import java.awt.event.*;
import javax.swing.*;

public class ResetAction extends AbstractAction {

    ViewerApp appInst;

    public ResetAction(ViewerApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        appInst.reset();
    }

}

