/**
 * TabListener
 *
 * clears selection when a tab is changed
 */

import java.util.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;

public class TabListener implements ChangeListener {

    public Set<ListSelectionModel> models;

    public TabListener() {
        models = new HashSet<ListSelectionModel>();
    }

    public void add(JTable table) {
        models.add(table.getSelectionModel());
    }

    public void stateChanged(ChangeEvent e) {
        for (ListSelectionModel m : models) {
            m.clearSelection();
        }
    }

}

