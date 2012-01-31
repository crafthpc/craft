/**
 * LogFilter
 *
 * only allow logfiles
 */

import java.io.*;

public class LogFilter extends javax.swing.filechooser.FileFilter {

    public String getExtension(File f) {
        String ext = null;
        String s = f.getName();
        int i = s.lastIndexOf('.');
        if (i > 0 &&  i < s.length() - 1) {
            ext = s.substring(i+1).toLowerCase();
        }
        return ext;
    }

    public boolean accept(File f) {
        if (f.isDirectory()) {
            return true;
        }
        String ext = getExtension(f);
        if (ext != null) {
            if (ext.equals("log")) {
                return true;
            }
        }
        return false;
    }

    public String getDescription() {
        return "Log files (*.log)";
    }
}

