/**
 * VariableListener
 *
 * blanks source code when a variable is selected
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class VariableListener extends MouseAdapter implements ListSelectionListener {

    private JTable variableList;
    ViewerApp appInst;

    private final DefaultListModel BLANK_LIST = new DefaultListModel();

    public VariableListener(JTable log, ViewerApp app) {
        variableList = log;
        appInst = app;
    }

    public void valueChanged(ListSelectionEvent e) {
        ListSelectionModel lsm = (ListSelectionModel)e.getSource();
        if (variableList.getModel() instanceof ShadowValueModel) {
            ShadowValueModel imodel = (ShadowValueModel)variableList.getModel();
            if (lsm.isSelectionEmpty() || imodel == null) {
                appInst.clearTopPanel();
            } else {
                LShadowValue val = imodel.values.get(lsm.getMinSelectionIndex());
                appInst.viewShadowValue(val);
            }
        } else {
            appInst.clearTopPanel();
        }
    }

    public void mousePressed(MouseEvent e)
    {
        JTable table = (JTable)e.getSource();
        Point p = e.getPoint();
        int row = table.rowAtPoint(p);
        if (variableList.getModel() instanceof ShadowValueModel) {
            ShadowValueModel imodel = (ShadowValueModel)variableList.getModel();
            if (row >= 0 && 
                    imodel != null && e.getClickCount() == 2) {
                LShadowValue val = imodel.values.get(row);
                if (val != null && val.isAggregate()) {
                    JOptionPane.showMessageDialog(null, "Name: " + val.name +
                            "  (" + val.rows + "," + val.cols + ")\n" +
                            "Min abs error: " + val.minAbsError + "\n" +
                            "Max abs error: " + val.maxAbsError + "\n" +
                            "Sum abs error: " + val.sumAbsError + "\n" +
                            "Avg abs error: " + val.avgAbsError + "\n" +
                            "Min rel error: " + val.minRelError + "\n" +
                            "Max rel error: " + val.maxRelError + "\n" +
                            "Sum rel error: " + val.sumRelError + "\n" +
                            "Avg rel error: " + val.avgRelError + "\n");
                }
            }
        }
    }

}

