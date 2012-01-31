/**
 * TraceFrameListener
 *
 * loads source code when a stack trace frame is selected
 */

import javax.swing.*;
import javax.swing.event.*;

public class TraceFrameListener implements ListSelectionListener {

    ViewerApp appInst;

    private JList backtrace;
    //private JTextArea sourceCode;
    
    public TraceFrameListener(JList trace, ViewerApp app /*, JTextArea code*/) {
        backtrace = trace;
        appInst = app;
        //sourceCode = code;
    }

    public void valueChanged(ListSelectionEvent e) {
        ListSelectionModel lsm = (ListSelectionModel)e.getSource();
        if (backtrace.getModel() instanceof LTrace) {
            LTrace trace = (LTrace)backtrace.getModel();
            if (lsm.isSelectionEmpty() || trace == null) {
                appInst.clearTopPanel();
            } else {
                LFrame frame = trace.frames.get(lsm.getMinSelectionIndex());
                if (frame.file.equals(""))
                    appInst.clearTopPanel();
                else
                    appInst.openSourceFile(frame.file, frame.lineno);
            }
        } else {
            appInst.clearTopPanel();
        }
    }

}

