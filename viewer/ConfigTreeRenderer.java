/**
 * ConfigTreeRenderer.java
 *
 * draws an entry in the configuration
 *
 */

import java.awt.*;
import javax.swing.*;
import javax.swing.tree.*;

public class ConfigTreeRenderer extends DefaultTreeCellRenderer {

    private boolean showEffectiveStatus;

    public ConfigTreeRenderer() {
        showEffectiveStatus = true;
    }

    public boolean getShowEffectiveStatus() {
        return showEffectiveStatus;
    }

    public void setShowEffectiveStatus(boolean newValue) {
        showEffectiveStatus = newValue;
    }

    public Component getTreeCellRendererComponent(JTree tree, Object value, 
            boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus) {

        setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        if (value instanceof ConfigTreeNode) {
            ConfigTreeNode node = (ConfigTreeNode)value;
            setText(node.toString());
            ConfigTreeNode.CNStatus status = node.status;
            if (showEffectiveStatus) {
                status = node.getEffectiveStatus();
            }
            switch (status) {
                case IGNORE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_IGNORE);
                    setOpaque(true);        break;
                case SINGLE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_SINGLE);
                    setOpaque(true);        break;
                case DOUBLE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_DOUBLE);
                    setOpaque(true);        break;
                default:
                    setOpaque(false);       break;
            }
        } else {
            setText(value.toString());
            setOpaque(false);
        }
        if (sel) {
            setBorder(BorderFactory.createLineBorder(ConfigEditorApp.DEFAULT_COLOR_BORDER, 1));
        } else {
            setBorder(BorderFactory.createEmptyBorder(1,1,1,1));
        }
        return this;
    }

}

