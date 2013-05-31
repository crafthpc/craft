/**
 * SaveConfigAction
 *
 * start saving a configuration
 */

import java.awt.event.*;
import javax.swing.*;

public class SaveConfigAction extends AbstractAction {

    ConfigEditorApp appInst;

    public SaveConfigAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        appInst.saveConfigFile();
    }
}

