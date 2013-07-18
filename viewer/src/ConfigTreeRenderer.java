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
    private boolean showPrecision;

    public ConfigTreeRenderer() {
        showEffectiveStatus = true;
        showCodeCoverage = false;
        showError = false;
        showPrecision = false;
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

    public boolean getShowPrecision() {
        return showPrecision;
    }

    public void setShowPrecision(boolean newValue) {
        showPrecision = newValue;
    }

    public Component getTreeCellRendererComponent(JTree tree, Object value, 
            boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus) {

        setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        if (value instanceof ConfigTreeNode) {
            ConfigTreeNode node = (ConfigTreeNode)value;
            setText(node.toString(showCodeCoverage, showError, showPrecision));
            ConfigTreeNode.CNStatus status = node.status;
            if (showEffectiveStatus) {
                status = node.getEffectiveStatus();
            }
            switch (status) {
                case IGNORE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_IGNORE);
                    setOpaque(true);        break;
                case CANDIDATE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_CANDIDATE);
                    setOpaque(true);        break;
                case SINGLE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_SINGLE);
                    setOpaque(true);        break;
                case DOUBLE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_DOUBLE);
                    setOpaque(true);        break;
                case TRANGE:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_TRANGE);
                    setOpaque(true);        break;
                case RPREC:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_RPREC);
                    setOpaque(true);        break;
                case NULL:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_NULL);
                    setOpaque(true);        break;
                case CINST:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_CINST);
                    setOpaque(true);        break;
                case DCANCEL:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_DCANCEL);
                    setOpaque(true);        break;
                case DNAN:
                    setBackground(ConfigEditorApp.DEFAULT_COLOR_DNAN);
                    setOpaque(true);        break;
                default:
                    setOpaque(false);       break;
            }
            if (showError) {
                // TODO: this should be empirically derived
                float err = ((float)Math.log10(node.error)+13.0f)/14.0f;
                setBackground(Util.getGreenRedScaledColor(err));
                setOpaque(true);
            }
            if (showPrecision) {
                setBackground(Util.getPrecisionScaledColor(node.precision));
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

