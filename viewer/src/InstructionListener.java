/**
 * InstructionListener
 *
 * loads source code when an instruction is selected
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class InstructionListener extends MouseAdapter implements ListSelectionListener {

    private JTable instructionList;
    ViewerApp appInst;
    //private JTextArea detailsLabel;
    
    private final DefaultListModel BLANK_LIST = new DefaultListModel();

    public InstructionListener(JTable log, ViewerApp app /*, JTextArea details*/) {
        instructionList = log;
        appInst = app;
        //detailsLabel = details;
    }

    public void valueChanged(ListSelectionEvent e) {
        ListSelectionModel lsm = (ListSelectionModel)e.getSource();
        if (instructionList.getModel() instanceof InstructionModel) {
            InstructionModel imodel = (InstructionModel)instructionList.getModel();
            if (lsm.isSelectionEmpty()) {
                appInst.clearTopPanel();
            } else {
                LInstruction instr = imodel.instructions.get(lsm.getMinSelectionIndex());
                //detailsLabel.setText(msg.details +
                        //(msg.instruction == null ? "" : 
                         //System.getProperty("line.separator") + msg.instruction.toString()));
                if (!(instr.file.equals(""))) {
                    appInst.openSourceFile(instr.file, instr.lineno);
                }
                else {
                    appInst.clearTopPanel();
                }
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
        if (instructionList.getModel() instanceof InstructionModel) {
            InstructionModel imodel = (InstructionModel)instructionList.getModel();
            if (row >= 0 && 
                    imodel != null && e.getClickCount() == 2) {
                LInstruction val = imodel.instructions.get(row);
                if (val != null) {
                    appInst.filterByInstruction(val);
                }
            }
        }
    }

}

