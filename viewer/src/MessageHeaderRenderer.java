/**
 * MessageHeaderRenderer
 *
 * handles drawing of message list column headers (show sorting order)
 */

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;

public class MessageHeaderRenderer extends JLabel implements TableCellRenderer {

    private MessageModel columnNameInfo;
    private TableHeaderListener sortInfo;

    public MessageHeaderRenderer(TableHeaderListener listener) {
        sortInfo = listener;
        setBorder(BorderFactory.createRaisedBevelBorder());
    }

    public JComponent getTableCellRendererComponent(JTable table, Object value, 
            boolean isSelected, boolean hasFocus, int row, int column) {
        String caption = value.toString();
        if (((MessageModel)table.getModel()).getColumnName(sortInfo.getSortIndex()).equals(caption)) {
            caption += (sortInfo.isAscending() ? " ^" : " v");
        }
        setText(caption);
        return this;
    }

}

