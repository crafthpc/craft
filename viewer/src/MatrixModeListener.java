/**
 * MatrixModeListener
 *
 * switch matrix viewing modes
 */

import java.awt.event.*;
import javax.swing.*;

public class MatrixModeListener implements ActionListener {

    public static final String SHADOW_LBL = "Shadow value";
    public static final String SHADOW_VAL = "Shadow value (in floating-point)";
    public static final String SYSTEM_VAL = "System value";
    public static final String ABS_ERROR = "Absolute error";
    public static final String REL_ERROR = "Relative error";

    private static final String[] VALID_OPTIONS = 
        { SHADOW_LBL, SHADOW_VAL, SYSTEM_VAL, ABS_ERROR, REL_ERROR };

    private static String mainMode = SHADOW_LBL;

    private JTable table;

    public static String[] getValidOptions() {
        return VALID_OPTIONS.clone();
    }

    public static String getMainMode() {
        return mainMode;
    }

    public static void setMainMode(String newMode) {
        mainMode = newMode;
    }

    public MatrixModeListener(JTable table) {
        this.table = table;
    }

    public static String getValue(LShadowValue val) {
        if (getMainMode().equals(SHADOW_LBL)) {
            return val.label;
        } else if (getMainMode().equals(SHADOW_VAL)) {
            return val.shadowValue;
        } else if (getMainMode().equals(SYSTEM_VAL)) {
            return val.systemValue;
        } else if (getMainMode().equals(ABS_ERROR)) {
            return val.absError;
        } else if (getMainMode().equals(REL_ERROR)) {
            return val.relError;
        } else {
            return "";
        }
    }

    public void actionPerformed(ActionEvent e) {
        if (e.getSource() instanceof JComboBox) {
            JComboBox box = (JComboBox)e.getSource();
            if (box.getSelectedItem() instanceof String) {
                String newMode = (String)box.getSelectedItem();
                if (newMode != null) {
                    if (newMode.equals(SHADOW_LBL) ||
                        newMode.equals(SHADOW_VAL) ||
                        newMode.equals(SYSTEM_VAL) ||
                        newMode.equals(ABS_ERROR) ||
                        newMode.equals(REL_ERROR)) {
                        setMainMode(newMode);
                        ColumnResizer.resizeColumns(table);
                        table.revalidate();
                        table.repaint();
                    }
                }
            }
        }
    }

}

