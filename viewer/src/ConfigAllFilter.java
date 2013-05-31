/**
 * ConfigFilter
 *
 * only allow config-related files: configs, logs, etc.
 */

import java.io.*;

public class ConfigAllFilter extends javax.swing.filechooser.FileFilter {

    public boolean accept(File f) {
        if (f.isDirectory()) {
            return true;
        }
        String ext = Util.getExtension(f);
        if (ext != null) {
            if (ext.equals("cfg") ||
                ext.equals("log") ||
                ext.equals("tested")) {
                return true;
            }
        }
        return false;
    }

    public String getDescription() {
        return "Config-related files (*.cfg;*.log;*.tested)";
    }
}

