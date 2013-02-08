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
    private boolean showCodeCoverage;
    private boolean showError;

    public ConfigTreeRenderer() {
        showEffectiveStatus = true;
        showCodeCoverage = false;
        showError = false;
    }

    public boolean getShowEffectiveStatus() {
        return showEffectiveStatus;
    }

    public void setShowEffectiveStatus(boolean newValue) {
        showEffectiveStatus = newValue;
    }

    public boolean getShowCodeCoverage() {
        return showCodeCoverage;
    }

    public void setShowCodeCoverage(boolean newValue) {
        showCodeCoverage = newValue;
    }

    public boolean getShowError() {
        return showError;
    }

    public void setShowError(boolean newValue) {
        showError = newValue;
    }

    public Component getTreeCellRendererComponent(JTree tree, Object value, 
            boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus) {

        setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        if (value instanceof ConfigTreeNode) {
            ConfigTreeNode node = (ConfigTreeNode)value;
            setText(node.toString(showCodeCoverage, showError));
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
                case CANDIDATE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_CANDIDATE);
                    setOpaque(true);        break;
                default:
                    setOpaque(false);       break;
            }
            if (showError) {
                // TODO: this should be empirically derived
                double err = (Math.log10(node.error)+13.0)/14.0;
                if (err > 1.0) err = 1.0;
                if (err < 0.0) err = 0.0;
                setBackground(new Color((float)err,(float)1.0-(float)err,(float)0.0));
                setOpaque(true);
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

