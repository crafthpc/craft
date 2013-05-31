/**
 * ProgFilter
 *
 * only allow programs
 */

import java.io.*;

public class ProgFilter extends javax.swing.filechooser.FileFilter {

    public boolean accept(File f) {
        if (f.isDirectory()) {
            return true;
        }
        // TODO: check for executability?
        return true;
    }

    public String getDescription() {
        return "Programs";
    }

}

