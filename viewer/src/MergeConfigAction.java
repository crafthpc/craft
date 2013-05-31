/**
 * MergeConfigAction
 *
 * display dialog and start merging a file (config, log, tested, etc.)
 */

import java.awt.event.*;
import java.io.*;
import javax.swing.*;

public class MergeConfigAction extends AbstractAction {

    ConfigEditorApp appInst;

    public MergeConfigAction(ConfigEditorApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        int returnVal;
        returnVal = appInst.fileAllChooser.showOpenDialog(appInst);

        if (returnVal == JFileChooser.APPROVE_OPTION) {
            File f = appInst.fileAllChooser.getSelectedFile();
            if (Util.getExtension(f).equals("cfg")) {
                appInst.mergeConfigFile(f);
            } else if (Util.getExtension(f).equals("log")) {
                appInst.mergeLogFile(f);
            } else if (Util.getExtension(f).equals("tested")) {
                appInst.mergeTestedFile(f);
            }
        }
    }
}

