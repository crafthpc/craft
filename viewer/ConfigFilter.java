/**
 * ConfigFilter
 *
 * only allow logfiles
 */

import java.io.*;

public class ConfigFilter extends javax.swing.filechooser.FileFilter {

    public boolean accept(File f) {
        if (f.isDirectory()) {
            return true;
        }
        String ext = Util.getExtension(f);
        if (ext != null) {
            if (ext.equals("cfg")) {
                return true;
            }
        }
        return false;
    }

    public String getDescription() {
        return "Configuration files (*.cfg)";
    }
}

