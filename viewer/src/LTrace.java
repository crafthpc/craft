/**
 * LTrace
 *
 * represents a stack trace; maintains a list of frames
 */

import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

public class LTrace implements ListModel {

    public String id;
    public java.util.List<LFrame> frames;

    public LTrace() {
        id = "";
        frames = new ArrayList<LFrame>();
    }

    public void addListDataListener(ListDataListener l) {}
    public void removeListDataListener(ListDataListener l) {}

    public Object getElementAt(int index) {
        return "   " + frames.get(index);
    }

    public int getSize() {
        return frames.size();
    }

    public String toString() {
        StringBuffer str = new StringBuffer();
        for (LFrame frame : frames) {
            str.append("\n   " + frame.toString());
        }
        return str.toString();
    }

}

