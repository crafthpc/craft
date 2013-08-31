/**
 * ImageFilter
 *
 * only allow image (*.png, specifically) files
 */

import java.io.*;

public class ImageFilter extends javax.swing.filechooser.FileFilter {

    public boolean accept(File f) {
        if (f.isDirectory()) {
            return true;
        }
        String ext = Util.getExtension(f);
        if (ext != null) {
            if (ext.equals("png")) {
                return true;
            }
        }
        return false;
    }

    public String getDescription() {
        return "Portable Network Graphics files (*.png)";
    }
}

