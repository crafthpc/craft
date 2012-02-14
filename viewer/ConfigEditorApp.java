/**
 * ConfigEditorApp.java
 *
 * Editor for configuration files.
 *
 * Original author:
 *   Mike Lam (lam@cs.umd.edu)
 *   Professor Jeffrey K. Hollingsworth, UMD
 *   January 2012
 */

// {{{ imports
import java.io.*;
import java.text.*;
import java.util.*;
import java.util.regex.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.table.*;
import javax.swing.tree.*;
// }}}

public class ConfigEditorApp extends JFrame implements ActionListener, DocumentListener {

    // {{{ member variables

    // shared interface elements
    public static final String DEFAULT_TITLE = "FPAnalysis Config Editor";
    public static final String FPCONF_PATH = "fpconf";
    public static final Font DEFAULT_FONT_SANS_PLAIN = new Font("SansSerif",  Font.PLAIN, 10);
    public static final Font DEFAULT_FONT_SANS_BOLD  = new Font("SansSerif",  Font.BOLD,  10);
    public static final Font DEFAULT_FONT_MONO_PLAIN = new Font("Monospaced", Font.PLAIN, 12);
    public static final Font DEFAULT_FONT_MONO_BOLD  = new Font("Monospaced", Font.BOLD,  12);
    public static final Color DEFAULT_COLOR_IGNORE = new Color(225, 225,   0);
    public static final Color DEFAULT_COLOR_SINGLE = new Color(125, 255, 125);
    public static final Color DEFAULT_COLOR_DOUBLE = new Color(255, 120, 120);
    public static final Color DEFAULT_COLOR_BORDER = new Color(  0,   0, 225);

    // main data structures
    public File mainConfigFile;
    public java.util.List<ConfigTreeNode> mainConfigEntries;
    public java.util.List<String> mainConfigMiscEntries;

    // main interface elements
    public JFileChooser fileChooser;
    public JPanel mainPanel;
    public JPanel topPanel;
    public JLabel filenameLabel;
    public JTextField searchBox;
    public JButton expandAllButton;
    public JButton expandDoubleButton;
    public JButton collapseAllButton;
    public JCheckBox showEffectiveBox;
    public JButton toggleButton;
    public JButton saveButton;
    public JTree mainTree;
    public ConfigTreeRenderer mainRenderer;
    public JLabel noneKeyLabel;
    public JLabel ignoreKeyLabel;
    public JLabel singleKeyLabel;
    public JLabel doubleKeyLabel;
    public JPanel bottomPanel;

    // }}}
    
    // {{{ general initialization

    public ConfigEditorApp() {
        super("");

        fileChooser = new JFileChooser();
        fileChooser.setCurrentDirectory(new File("."));
        fileChooser.addChoosableFileFilter(new ConfigFilter());

        buildMenuBar();
        buildMainInterface();
    }

    // }}}
    // {{{ initialize main interface

    public void buildMenuBar() {
        JMenu logMenu = new JMenu("Actions");
        logMenu.setMnemonic(KeyEvent.VK_A);
        logMenu.add(new OpenConfigAction(this, "Open", null, new Integer(KeyEvent.VK_O)));
        logMenu.add(new SaveConfigAction(this, "Save", null, new Integer(KeyEvent.VK_M)));
        logMenu.add(new JSeparator());
        logMenu.add(new BatchConfigAction(this, "Batch Config", null, new Integer(KeyEvent.VK_M)));
        
        JMenuBar menuBar = new JMenuBar();
        menuBar.add(logMenu);
        setJMenuBar(menuBar);
    }

    public void buildMainInterface() {

        topPanel = new JPanel();
        filenameLabel = new JLabel("");
        //topPanel.add(filenameLabel);  // kind of redundant; it's in the window title
        topPanel.add(new JLabel("   "));
        topPanel.add(new JLabel("Search:"));
        searchBox = new JTextField(10);
        searchBox.addActionListener(this);
        searchBox.getDocument().addDocumentListener(this);
        topPanel.add(searchBox);
        topPanel.add(new JLabel("   "));
        expandAllButton = new JButton("Expand all");
        expandAllButton.addActionListener(this);
        topPanel.add(expandAllButton);
        collapseAllButton = new JButton("Collapse all");
        collapseAllButton.addActionListener(this);
        topPanel.add(collapseAllButton);
        expandDoubleButton = new JButton("View doubles");
        expandDoubleButton.addActionListener(this);
        topPanel.add(expandDoubleButton);
        topPanel.add(new JLabel("   "));
        showEffectiveBox = new JCheckBox("Show effective status");
        showEffectiveBox.addActionListener(this);
        topPanel.add(showEffectiveBox);

        mainTree = new JTree();
        mainRenderer = new ConfigTreeRenderer();
        showEffectiveBox.setSelected(mainRenderer.getShowEffectiveStatus());
        mainTree.setCellRenderer(mainRenderer);
        ConfigTreeListener ctl = new ConfigTreeListener(this, mainTree);
        mainTree.addMouseListener(ctl);
        mainTree.addTreeExpansionListener(ctl);
        mainTree.setModel(new DefaultTreeModel(new ConfigTreeNode()));

        bottomPanel = new JPanel();
        bottomPanel.setBackground(Color.WHITE);
        bottomPanel.add(new JLabel("Instruction counts:  "));
        noneKeyLabel = new JLabel("NONE");
        noneKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        bottomPanel.add(noneKeyLabel);
        bottomPanel.add(new JLabel("  "));
        ignoreKeyLabel = new JLabel("IGNORE");
        ignoreKeyLabel.setBackground(DEFAULT_COLOR_IGNORE);
        ignoreKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        ignoreKeyLabel.setOpaque(true);
        bottomPanel.add(ignoreKeyLabel);
        bottomPanel.add(new JLabel("  "));
        singleKeyLabel = new JLabel("SINGLE");
        singleKeyLabel.setBackground(DEFAULT_COLOR_SINGLE);
        singleKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        singleKeyLabel.setOpaque(true);
        bottomPanel.add(singleKeyLabel);
        bottomPanel.add(new JLabel("  "));
        doubleKeyLabel = new JLabel("DOUBLE");
        doubleKeyLabel.setBackground(DEFAULT_COLOR_DOUBLE);
        doubleKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        doubleKeyLabel.setOpaque(true);
        bottomPanel.add(doubleKeyLabel);
        refreshKeyLabels();

        bottomPanel.add(new JLabel("      "));
        toggleButton = new JButton("Toggle selection");
        toggleButton.addActionListener(this);
        bottomPanel.add(toggleButton);
        saveButton = new JButton("Save");
        saveButton.addActionListener(this);
        bottomPanel.add(saveButton);

        mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(new JScrollPane(mainTree), BorderLayout.CENTER);
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);

        getContentPane().add(mainPanel);
        setSize(1000,900);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);
        setTitle(DEFAULT_TITLE);

        mainPanel.revalidate();
    }

    // }}}

    // {{{ helper/utility functions

    public void openFile(File file) {
        if (file.getName().endsWith(".cfg")) {

            // config file; open as usual
            openConfigFile(file);

        } else {

            // this is a binary; use a config file in the current directory
            // with the same base name
            File cfgFile = new File(file.getName() + ".cfg");
            String cmd = FPCONF_PATH + " " + file.getAbsolutePath();
            try {

                // if the config file already exists, prompt for overwrite confirmation
                if (cfgFile.exists()) {
                    int rval = JOptionPane.showConfirmDialog(this,
                            "\"" + cfgFile.getName() + "\" already exists. Regenerate it?",
                            DEFAULT_TITLE, JOptionPane.YES_NO_OPTION);
                    if (rval == JOptionPane.NO_OPTION) {
                        openConfigFile(cfgFile);
                        return;
                    }
                }

                // run fpconf to generate initial configuration
                cfgFile.createNewFile();
                Process proc = Runtime.getRuntime().exec(cmd);
                BufferedReader rdr = new BufferedReader(
                        new InputStreamReader(proc.getInputStream()));
                PrintWriter wrt = new PrintWriter(cfgFile);
                String line;
                while ((line = rdr.readLine()) != null) {
                    wrt.println(line);
                }
                proc.waitFor();
                wrt.close();
                rdr.close();

                if (proc.exitValue() == 0) {
                    // open the new config file
                    openConfigFile(cfgFile);
                }

            } catch (SecurityException e) {
                System.err.println("ERROR (Security): " + e.getMessage());
            } catch (IOException e) {
                System.err.println("ERROR (I/O): " + e.getMessage());
            } catch (InterruptedException e) {
                System.err.println("ERROR (Proc): " + e.getMessage());
            }
        }
    }

    public void openConfigFile(File file) {
        mainConfigFile = file;
        mainConfigEntries = new ArrayList<ConfigTreeNode>();
        mainConfigMiscEntries = new ArrayList<String>();

        ConfigTreeNode appNode = new ConfigTreeNode();
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;
        String curLine;

        try {
            BufferedReader rdr = new BufferedReader(new FileReader(mainConfigFile));

            // scan the file and build the configuration structure
            while ((curLine = rdr.readLine()) != null) {
                if (curLine.startsWith("^")) {
                    curNode = new ConfigTreeNode(curLine);
                    if (curNode.type == ConfigTreeNode.CNType.APPLICATION) {
                        appNode = curNode;
                        mainConfigEntries.add(curNode);
                    } else if (curNode.type == ConfigTreeNode.CNType.FUNCTION) {
                        appNode.add(curNode);
                        curFuncNode = curNode;
                        mainConfigEntries.add(curNode);
                    } else if (curNode.type == ConfigTreeNode.CNType.BASIC_BLOCK &&
                               curFuncNode != null) {
                        if (curBlockNode != null) {
                            // clean previous block (remove if it has no children)
                            if (curBlockNode.getChildCount() == 0) {
                                TreeNode parent = curBlockNode.getParent();
                                if (parent instanceof ConfigTreeNode) {
                                    ((ConfigTreeNode)parent).remove(curBlockNode);
                                }
                            }
                        }
                        curFuncNode.add(curNode);
                        curBlockNode = curNode;
                        mainConfigEntries.add(curNode);
                    } else if (curNode.type == ConfigTreeNode.CNType.INSTRUCTION &&
                               curBlockNode != null) {
                        curBlockNode.add(curNode);
                        mainConfigEntries.add(curNode);
                    }
                } else {
                    mainConfigMiscEntries.add(curLine);
                }
            }

            rdr.close();
        } catch (IOException e) {
            System.err.println("ERROR: " + e.getMessage());
        }

        setTitle(DEFAULT_TITLE + " - " + mainConfigFile.getName());
        filenameLabel.setText(mainConfigFile.getName());
        TreeModel model = new DefaultTreeModel(appNode);
        mainTree.setModel(model);
        refreshKeyLabels();
    }
    
    public void saveConfigFile() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;


        try {
            PrintWriter wrt = new PrintWriter(mainConfigFile);
            //PrintWriter wrt = new PrintWriter("test.txt");        // for testing

            // re-emit other config entries
            for (String s : mainConfigMiscEntries) {
                wrt.println(s);
            }

            // walk the config tree and emit all settings
            /*
             *ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
             *ConfigTreeNode curFuncNode = null;
             *ConfigTreeNode curBlockNode = null;
             *ConfigTreeNode curNode = null;
             *wrt.println(appNode.formatFileEntry());
             *Enumeration<ConfigTreeNode> funcs = appNode.children();
             *while (funcs.hasMoreElements()) {
             *   curFuncNode = funcs.nextElement();
             *   wrt.println(curFuncNode.formatFileEntry());
             *   Enumeration<ConfigTreeNode> blocks = curFuncNode.children();
             *   while (blocks.hasMoreElements()) {
             *       curBlockNode = blocks.nextElement();
             *       wrt.println(curBlockNode.formatFileEntry());
             *       Enumeration<ConfigTreeNode> insns = curBlockNode.children();
             *       while (insns.hasMoreElements()) {
             *           curNode = insns.nextElement();
             *           wrt.println(curNode.formatFileEntry());
             *       }
             *   }
             *}
             */

            // emit all config entries
            for (ConfigTreeNode node : mainConfigEntries) {
                wrt.println(node.formatFileEntry());
            }

            wrt.close();
        } catch (IOException e) {
            System.err.println("ERROR: " + e.getMessage());
        }
    }

    public void refreshKeyLabels() {
        int noneCount   = 0;
        int ignoreCount = 0;
        int singleCount = 0;
        int doubleCount = 0;
        ConfigTreeNode.CNStatus status;
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;
        Enumeration<ConfigTreeNode> funcs = appNode.children();
        while (funcs.hasMoreElements()) {
            curFuncNode = funcs.nextElement();
            Enumeration<ConfigTreeNode> blocks = curFuncNode.children();
            while (blocks.hasMoreElements()) {
                curBlockNode = blocks.nextElement();
                Enumeration<ConfigTreeNode> insns = curBlockNode.children();
                while (insns.hasMoreElements()) {
                    curNode = insns.nextElement();
                    if (mainRenderer.getShowEffectiveStatus()) {
                        status = curNode.getEffectiveStatus();
                    } else {
                        status = curNode.status;
                    }
                    switch (status) {
                        case NONE:      noneCount++;        break;
                        case IGNORE:    ignoreCount++;      break;
                        case SINGLE:    singleCount++;      break;
                        case DOUBLE:    doubleCount++;      break;
                        default:                            break;
                    }
                }
            }
        }
        noneKeyLabel.setText("NONE (" + noneCount + ")");
        ignoreKeyLabel.setText("IGNORE (" + ignoreCount + ")");
        singleKeyLabel.setText("SINGLE (" + singleCount + ")");
        doubleKeyLabel.setText("DOUBLE (" + doubleCount + ")");
    }

    public void search(boolean findNext) {
        TreePath path;
        Object obj;
        ConfigTreeNode node;
        String text = searchBox.getText();

        // generally, start at the top ...
        int start = 0;

        // ... but if we're looking for the "next" match,
        // start after the current selection
        if (findNext && mainTree.getSelectionCount() > 0) {
            start = (mainTree.getMaxSelectionRow() + 1) % mainTree.getRowCount();
        }

        int i = start;
        boolean keepSearching = true;
        while (keepSearching) {

            // grab the tree node at index i
            path = mainTree.getPathForRow(i);
            obj = path.getLastPathComponent();
            if (obj instanceof ConfigTreeNode) {
                node = (ConfigTreeNode)obj;

                // check for search text
                if (node.label.contains(text)) {

                    // we found a valid result
                    mainTree.scrollRowToVisible(i);
                    mainTree.setSelectionRow(i);
                    keepSearching = false;
                }
            }

            // next index (with wrap to top)
            i = (i+1) % mainTree.getRowCount();

            if (i == start) {
                // we're back where we started; stop searching
                keepSearching = false;
                mainTree.clearSelection();
            }
        }
    }

    public void batchConfig(ConfigTreeNode.CNStatus orig, ConfigTreeNode.CNStatus dest) {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;
        Enumeration<ConfigTreeNode> funcs = appNode.children();
        while (funcs.hasMoreElements()) {
            curFuncNode = funcs.nextElement();
            Enumeration<ConfigTreeNode> blocks = curFuncNode.children();
            while (blocks.hasMoreElements()) {
                curBlockNode = blocks.nextElement();
                Enumeration<ConfigTreeNode> insns = curBlockNode.children();
                while (insns.hasMoreElements()) {
                    curNode = insns.nextElement();
                    if (curNode.status == orig) {
                        curNode.status = dest;
                    }
                }
            }
        }
        mainTree.repaint();
        refreshKeyLabels();
    }

    public void expandAllRows() {
        for (int i = 0; i < mainTree.getRowCount(); i++) {
            mainTree.expandRow(i);
        }
    }

    public void expandDoubleRows() {
        for (int i = 0; i < mainTree.getRowCount(); i++) {
            ConfigTreeNode curNode = (ConfigTreeNode)mainTree.getPathForRow(i).getLastPathComponent();
            if (curNode.type == ConfigTreeNode.CNType.FUNCTION &&
                curNode.status == ConfigTreeNode.CNStatus.NONE &&
                    shouldExpandDoubleRow(curNode)) {
                mainTree.expandRow(i);
            } else if (curNode.type == ConfigTreeNode.CNType.BASIC_BLOCK &&
                       curNode.status == ConfigTreeNode.CNStatus.NONE &&
                    shouldExpandDoubleRow(curNode)) {
                mainTree.expandRow(i);
            } else if (curNode.type != ConfigTreeNode.CNType.APPLICATION) {
                mainTree.collapseRow(i);
            }
        }
    }

    public boolean shouldExpandDoubleRow(TreeNode parent) {
        if (parent == null || !(parent instanceof ConfigTreeNode)) return false;
        ConfigTreeNode child = null;
        Enumeration<ConfigTreeNode> children = parent.children();
        while (children.hasMoreElements()) {
            child = children.nextElement();
            if (child.status == ConfigTreeNode.CNStatus.DOUBLE ||
                    shouldExpandDoubleRow(child)) {
                return true;
            }
        }
        return false;
    }

    public void collapseAllRows() {
        for (int i = mainTree.getRowCount() - 1; i > 0; i--) {
            mainTree.collapseRow(i);
        }
    }

    public void toggleSelection() {
        TreePath[] paths = mainTree.getSelectionPaths();
        if (paths != null) {
            for (int i = 0; i < paths.length; i++) {
                TreePath selPath = paths[i];
                Object obj = selPath.getLastPathComponent();
                if (obj instanceof ConfigTreeNode) {
                    ((ConfigTreeNode)obj).toggleAll();
                }
            }
            mainTree.repaint();
            refreshKeyLabels();
        }
    }

    // }}}

    // {{{ event listeners

    public void changedUpdate(DocumentEvent e) {
        search(false);
    }

    public void insertUpdate(DocumentEvent e) {
        search(false);
    }

    public void removeUpdate(DocumentEvent e) {
        search(false);
    }

    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == searchBox) {
            search(true);
        } else if (e.getSource() == expandAllButton) {
            expandAllRows();
        } else if (e.getSource() == expandDoubleButton) {
            expandDoubleRows();
        } else if (e.getSource() == collapseAllButton) {
            collapseAllRows();
        } else if (e.getSource() == showEffectiveBox) {
            mainRenderer.setShowEffectiveStatus(showEffectiveBox.isSelected());
            mainTree.repaint();
            refreshKeyLabels();
        } else if (e.getSource() == toggleButton) {
            toggleSelection();
        } else if (e.getSource() == saveButton) {
            saveConfigFile();
        }
    }

    // }}}

    // {{{ main method
    public static void main(String[] args) {
        java.util.List<File> files = new ArrayList<File>();
        int i;
        String tag;
        File instFile;

        // parse command-line args
        for (i = 0; i < args.length; i++) {
            files.add(new File(args[i]));
        }

        // open all given log files
        for (File f : files) {
            ConfigEditorApp app = new ConfigEditorApp();
            app.openFile(f);
            app.setVisible(true);
        }

        // empty config if no file is given
        if (files.size() == 0) {
            ConfigEditorApp app = new ConfigEditorApp();
            app.setVisible(true);
        }

    } // }}}

}

