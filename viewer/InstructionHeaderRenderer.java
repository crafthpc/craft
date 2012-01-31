/**
 * InstructionHeaderRenderer
 *
 * handles drawing of instruction list column headers (show sorting order)
 */

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;

public class InstructionHeaderRenderer extends JLabel implements TableCellRenderer {

    private TableHeaderListener sortInfo;

    public InstructionHeaderRenderer(TableHeaderListener listener) {
        sortInfo = listener;
        setBorder(BorderFactory.createRaisedBevelBorder());
    }

    public JComponent getTableCellRendererComponent(JTable table, Object value, 
            boolean isSelected, boolean hasFocus, int row, int column) {
        String caption = value.toString();
        if (((InstructionModel)table.getModel()).getColumnName(sortInfo.getSortIndex()).equals(caption)) {
            caption += (sortInfo.isAscending() ? " ^" : " v");
        }
        setText(caption);
        return this;
    }

}

