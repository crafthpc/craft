/**
 * ConfigTreeListener.java
 *
 * handles events fired by the main tree
 *
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;
import java.util.*;

public class ConfigTreeListener extends MouseAdapter implements TreeExpansionListener {

    private ConfigEditorApp mainApp;
    private JTree mainTree;

    public ConfigTreeListener(ConfigEditorApp app, JTree tree) {
        mainApp = app;
        mainTree = tree;
    }

    public void mousePressed(MouseEvent e) {
        int selRow = mainTree.getRowForLocation(e.getX(), e.getY());
        TreePath selPath = mainTree.getPathForLocation(e.getX(), e.getY());
        if (selRow != -1) {
            Object obj = selPath.getLastPathComponent();
            if (obj instanceof ConfigTreeNode) {
                if (e.getClickCount() == 1 && e.getButton() == MouseEvent.BUTTON2) {
                    ((ConfigTreeNode)obj).toggleNoneSingle();
                } else if (e.getClickCount() == 1 && e.getButton() == MouseEvent.BUTTON3) {
                    ((ConfigTreeNode)obj).toggleIgnoreSingle();
                } else if (e.getClickCount() == 2 && e.getButton() == MouseEvent.BUTTON1) {
                    mainApp.openSourceWindow((ConfigTreeNode)obj);
                }
            }
            mainTree.repaint();
            mainApp.refreshKeyLabels();
        }
    }

    public void treeCollapsed(TreeExpansionEvent e) {
    }

    public void treeExpanded(TreeExpansionEvent e) {
        TreePath selPath = e.getPath();
        Object obj = selPath.getLastPathComponent();
        if (obj instanceof ConfigTreeNode) {
            ConfigTreeNode node = (ConfigTreeNode)obj;
            if (node.type == ConfigTreeNode.CNType.FUNCTION) {
                Enumeration<ConfigTreeNode> children = node.children();
                while (children.hasMoreElements()) {
                    mainTree.expandPath(selPath.pathByAddingChild(children.nextElement()));
                }
            }
        }
    }

}

