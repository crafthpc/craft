/**
 * MatrixCellListener
 *
 * displays cell details
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class MatrixCellListener extends MouseAdapter {

    public LShadowValue model;

    public MatrixCellListener() {
        model = null;
    }

    public void setModel(LShadowValue model) {
        this.model = model;
    }

    public void mousePressed(MouseEvent e)
    {
        JTable table = (JTable)e.getSource();
        Point p = e.getPoint();
        if (model != null && e.getClickCount() == 2) {
            int row = table.rowAtPoint(p);
            int col = table.columnAtPoint(p);
            LShadowValue value = (LShadowValue)model.getSubValueAt(row,col);
            if (value != null) {
                JOptionPane.showMessageDialog(null, "Name: " + value.name + "\n" +
                        "Label: " + value.label + "\n" +
                        "Shadow value: " + value.shadowValue + "\n" +
                        "System value: " + value.systemValue + "\n" +
                        "Absolute error: " + value.absError + "\n" +
                        "Relative error: " + value.relError + "\n" +
                        "Address: " + value.address + "\n" +
                        "Size: " + value.size + "\n" +
                        "Shadow Type: " + value.shadowType);
            }
        }
    }

}

