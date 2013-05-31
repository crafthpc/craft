/**
 * TableHeaderListener
 *
 * sorts a table by column
 */

import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;

public class TableHeaderListener extends MouseAdapter {

    private JTable table;
    private int prevSortIndex;
    private boolean sortAsc;
    
    public TableHeaderListener(JTable messageList) {
        table = messageList;
        prevSortIndex = 0;
        sortAsc = true;
    }

    public int getSortIndex() {
        return prevSortIndex;
    }

    public boolean isAscending() {
        return sortAsc;
    }

    public void sortTable(int index) {
        prevSortIndex = index;
    }

    public void mouseClicked(MouseEvent e) {
        TableColumnModel colModel = table.getColumnModel();
        TableModel tblModel = table.getModel();
        int columnModelIndex = colModel.getColumnIndexAtX(e.getX());
        int modelIndex = colModel.getColumn(columnModelIndex).getModelIndex();
        if (modelIndex == prevSortIndex) {
            sortAsc = !sortAsc;
        } else {
            sortAsc = true;
        }
        prevSortIndex = modelIndex;
        if (tblModel instanceof MessageModel) {
            ((MessageModel)tblModel).sort(modelIndex, sortAsc);
        } else if (tblModel instanceof InstructionModel) {
            ((InstructionModel)tblModel).sort(modelIndex, sortAsc);
        } else if (tblModel instanceof ShadowValueModel) {
            ((ShadowValueModel)tblModel).sort(modelIndex, sortAsc);
        }
        table.revalidate();
        table.getTableHeader().revalidate();
    }

}

