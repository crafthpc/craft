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
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;
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
    public static final Color DEFAULT_COLOR_IGNORE    = new Color(225, 225,   0);
    public static final Color DEFAULT_COLOR_SINGLE    = new Color(125, 255, 125);
    public static final Color DEFAULT_COLOR_DOUBLE    = new Color(255, 120, 120);
    public static final Color DEFAULT_COLOR_CANDIDATE = new Color(125, 175, 225);
    public static final Color DEFAULT_COLOR_BORDER    = new Color(  0,   0, 225);

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
    public JButton expandSingleButton;
    public JButton expandDoubleButton;
    public JButton collapseAllButton;
    public JCheckBox showEffectiveBox;
    public JButton toggleButton;
    public JButton setNoneButton;
    public JButton setIgnoreButton;
    public JButton setSingleButton;
    public JButton setDoubleButton;
    public JButton setCandidateButton;
    public JButton saveButton;
    public JTree mainTree;
    public ConfigTreeRenderer mainRenderer;
    public JLabel noneKeyLabel;
    public JLabel ignoreKeyLabel;
    public JLabel singleKeyLabel;
    public JLabel doubleKeyLabel;
    public JLabel candidateKeyLabel;
    public JPanel bottomPanel;

    // }}}
    
    // {{{ general initialization

    public ConfigEditorApp() {
        super("");

        fileChooser = new JFileChooser();
        fileChooser.setCurrentDirectory(new File("."));
        fileChooser.addChoosableFileFilter(new ConfigFilter());

        Util.initSearchDirs();

        buildMenuBar();
        buildMainInterface();
    }

    // }}}
    // {{{ initialize main interface

    public void buildMenuBar() {
        JMenu logMenu = new JMenu("Actions");
        logMenu.setMnemonic(KeyEvent.VK_A);
        logMenu.add(new OpenConfigAction(this, "Open", null, new Integer(KeyEvent.VK_O)));
        logMenu.add(new SaveConfigAction(this, "Save", null, new Integer(KeyEvent.VK_S)));
        logMenu.add(new JSeparator());
        logMenu.add(new BatchConfigAction(this, "Batch Config", null, new Integer(KeyEvent.VK_M)));
        logMenu.add(new RemoveNonExecutedAction(this, "Remove Non-executed Entries", null, new Integer(KeyEvent.VK_N)));
        
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
        expandSingleButton = new JButton("View singles");
        expandSingleButton.addActionListener(this);
        topPanel.add(expandSingleButton);
        expandDoubleButton = new JButton("View doubles");
        expandDoubleButton.addActionListener(this);
        topPanel.add(expandDoubleButton);
        topPanel.add(new JLabel("   "));
        showEffectiveBox = new JCheckBox("Show effective status");
        showEffectiveBox.addActionListener(this);
        topPanel.add(showEffectiveBox);
        topPanel.add(new JLabel("   "));
        saveButton = new JButton("Save");
        saveButton.addActionListener(this);
        topPanel.add(saveButton);

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
        bottomPanel.add(new JLabel("  "));
        candidateKeyLabel = new JLabel("CANDIDATE");
        candidateKeyLabel.setBackground(DEFAULT_COLOR_CANDIDATE);
        candidateKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        candidateKeyLabel.setOpaque(true);
        bottomPanel.add(candidateKeyLabel);
        refreshKeyLabels();

        bottomPanel.add(new JLabel("      "));
        toggleButton = new JButton("Toggle selection");
        toggleButton.addActionListener(this);
        //bottomPanel.add(toggleButton);
        setNoneButton = new JButton(" ");
        setNoneButton.setBackground(Color.WHITE);
        setNoneButton.addActionListener(this);
        bottomPanel.add(setNoneButton);
        setIgnoreButton = new JButton("I");
        setIgnoreButton.setBackground(DEFAULT_COLOR_IGNORE);
        setIgnoreButton.addActionListener(this);
        bottomPanel.add(setIgnoreButton);
        setSingleButton = new JButton("S");
        setSingleButton.setBackground(DEFAULT_COLOR_SINGLE);
        setSingleButton.addActionListener(this);
        bottomPanel.add(setSingleButton);
        setDoubleButton = new JButton("D");
        setDoubleButton.setBackground(DEFAULT_COLOR_DOUBLE);
        setDoubleButton.addActionListener(this);
        bottomPanel.add(setDoubleButton);
        setCandidateButton = new JButton("C");
        setCandidateButton.setBackground(DEFAULT_COLOR_CANDIDATE);
        setCandidateButton.addActionListener(this);
        bottomPanel.add(setCandidateButton);

        mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(new JScrollPane(mainTree), BorderLayout.CENTER);
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);

        getContentPane().add(mainPanel);
        setSize(1200,900);
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

    public ConfigTreeNode openConfigFile(File file) {
        mainConfigFile = file;
        mainConfigEntries = new ArrayList<ConfigTreeNode>();
        mainConfigMiscEntries = new ArrayList<String>();

        ConfigTreeNode appNode = new ConfigTreeNode();
        ConfigTreeNode curModNode = null;
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
                    } else if (curNode.type == ConfigTreeNode.CNType.MODULE) {
                        if (curModNode != null) {
                            // clean previous module (remove if it has no children)
                            if (curModNode.getInsnCount() == 0) {
                                TreeNode parent = curModNode.getParent();
                                if (parent instanceof ConfigTreeNode) {
                                    ((ConfigTreeNode)parent).remove(curModNode);
                                }
                            }
                        }
                        appNode.add(curNode);
                        curModNode = curNode;
                        mainConfigEntries.add(curNode);
                    } else if (curNode.type == ConfigTreeNode.CNType.FUNCTION) {
                        if (curFuncNode != null) {
                            // clean previous function (remove if it has no children)
                            if (curFuncNode.getInsnCount() == 0) {
                                TreeNode parent = curFuncNode.getParent();
                                if (parent instanceof ConfigTreeNode) {
                                    ((ConfigTreeNode)parent).remove(curFuncNode);
                                }
                            }
                        }
                        curModNode.add(curNode);
                        curFuncNode = curNode;
                        mainConfigEntries.add(curNode);
                    } else if (curNode.type == ConfigTreeNode.CNType.BASIC_BLOCK &&
                               curFuncNode != null) {
                        if (curBlockNode != null) {
                            // clean previous block (remove if it has no children)
                            if (curBlockNode.getInsnCount() == 0) {
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
        refreshTree(appNode);

        return appNode;
    }

    public void refreshTree(ConfigTreeNode appNode) {
        TreeModel model = new DefaultTreeModel(appNode);
        mainTree.setModel(model);
        mainTree.repaint();
        refreshKeyLabels();

        DefaultMutableTreeNode node = appNode.getNextNode();
        while (node != null) {
            if (node.getLevel() == 1) {
                mainTree.expandPath(new TreePath(node.getPath()));
            }
            node = node.getNextNode();
        }
    }

    public boolean mergeConfigFile(File file) {

        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return false;
        ConfigTreeNode origAppNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        ConfigTreeNode origModNode = null;
        ConfigTreeNode origFuncNode = null;
        ConfigTreeNode origBlockNode = null;

        ConfigTreeNode appNode = new ConfigTreeNode();
        ConfigTreeNode curModNode = null;
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;
        String curLine;

        boolean success = true;

        try {
            BufferedReader rdr = new BufferedReader(new FileReader(file));

            // scan the file and build the configuration structure
            while (success && (curLine = rdr.readLine()) != null) {
                if (curLine.startsWith("^")) {
                    curNode = new ConfigTreeNode(curLine);
                    if (curNode.type == ConfigTreeNode.CNType.APPLICATION) {
                        appNode = curNode;
                        if (!appNode.label.equals(origAppNode.label)) {
                            success = false;
                        } else {
                            origAppNode.merge(appNode);
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.MODULE) {
                        curModNode = curNode;
                        Enumeration<ConfigTreeNode> origModules = origAppNode.children();
                        boolean found = false;
                        while (origModules.hasMoreElements()) {
                            ConfigTreeNode mod = origModules.nextElement();
                            if (mod.label.equals(curModNode.label)) {
                                mod.merge(curModNode);
                                origModNode = mod;
                                found = true;
                            }
                        }
                        if (!found) {
                            success = false;
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.FUNCTION) {
                        curFuncNode = curNode;
                        Enumeration<ConfigTreeNode> origFuncs = origModNode.children();
                        boolean found = false;
                        while (origFuncs.hasMoreElements()) {
                            ConfigTreeNode func = origFuncs.nextElement();
                            if (func.label.equals(curFuncNode.label)) {
                                func.merge(curFuncNode);
                                origFuncNode = func;
                                found = true;
                            }
                        }
                        if (!found) {
                            success = false;
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.BASIC_BLOCK &&
                               curFuncNode != null && origFuncNode != null) {
                        curBlockNode = curNode;
                        Enumeration<ConfigTreeNode> origBlocks = origFuncNode.children();
                        boolean found = false;
                        while (origBlocks.hasMoreElements()) {
                            ConfigTreeNode block = origBlocks.nextElement();
                            if (block.label.equals(curBlockNode.label)) {
                                block.merge(curBlockNode);
                                origBlockNode = block;
                                found = true;
                            }
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.INSTRUCTION &&
                               curBlockNode != null && origBlockNode != null) {
                        Enumeration<ConfigTreeNode> origInsns = origBlockNode.children();
                        boolean found = false;
                        while (origInsns.hasMoreElements()) {
                            ConfigTreeNode insn = origInsns.nextElement();
                            if (insn.label.equals(curNode.label)) {
                                insn.merge(curNode);
                                found = true;
                            }
                        }
                    }
                } else {
                    // TODO: do something with misc config entries?
                    //mainConfigMiscEntries.add(curLine);
                }
            }

            rdr.close();
        } catch (IOException e) {
            System.err.println("ERROR: " + e.getMessage());
        }

        setTitle(DEFAULT_TITLE + " - (multiple files)");
        filenameLabel.setText("(multiple files)");
        return success;
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

            // emit all config entries
            for (ConfigTreeNode node : mainConfigEntries) {
                wrt.println(node.formatFileEntry());
            }

            wrt.close();
        } catch (IOException e) {
            System.err.println("ERROR: " + e.getMessage());
        }
    }

    public void mergeLogFile(File file) {
        if (!file.exists()) {
            JOptionPane.showMessageDialog(null, "I/O error: " + file.getAbsolutePath() + " does not exist!");
            return;
        }
        try {
            // parse the logfile
            SAXParserFactory factory = SAXParserFactory.newInstance();
            factory.setNamespaceAware(true);
            SAXParser parser = factory.newSAXParser();
            LogFileHandler fileLoader = new LogFileHandler();
            parser.parse(file, fileLoader);
            LLogFile logfile = fileLoader.getLogFile();
            InstructionModel im = new InstructionModel(logfile);
            im.refreshData();

            // walk the config tree and retrieve/calculate all execution counts
            ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
            ConfigTreeNode curModuleNode = null;
            ConfigTreeNode curFuncNode = null;
            ConfigTreeNode curBlockNode = null;
            ConfigTreeNode curNode = null;
            Enumeration<ConfigTreeNode> modules = appNode.children();
            while (modules.hasMoreElements()) {
                curModuleNode = modules.nextElement();
                Enumeration<ConfigTreeNode> funcs = curModuleNode.children();
                while (funcs.hasMoreElements()) {
                    curFuncNode = funcs.nextElement();
                    Enumeration<ConfigTreeNode> blocks = curFuncNode.children();
                    while (blocks.hasMoreElements()) {
                        curBlockNode = blocks.nextElement();
                        Enumeration<ConfigTreeNode> insns = curBlockNode.children();
                        while (insns.hasMoreElements()) {
                            curNode = insns.nextElement();
                            LInstruction insn = logfile.instructionsByAddress.get(curNode.address);
                            if (insn != null) {
                                long count = insn.count;
                                curNode.totalExecCount = count;
                                curNode.execCount.put(curNode.status, new Long(count));

                                // aggregate replacement status statistics
                                curBlockNode.totalExecCount += count;
                                if (curBlockNode.execCount.containsKey(curNode.status)) {
                                    curBlockNode.execCount.put(curNode.status,
                                            new Long(count) + curBlockNode.execCount.get(curNode.status));
                                } else {
                                    curBlockNode.execCount.put(curNode.status, new Long(count));
                                }
                                curFuncNode.totalExecCount += count;
                                if (curFuncNode.execCount.containsKey(curNode.status)) {
                                    curFuncNode.execCount.put(curNode.status,
                                            new Long(count) + curFuncNode.execCount.get(curNode.status));
                                } else {
                                    curFuncNode.execCount.put(curNode.status, new Long(count));
                                }
                                curModuleNode.totalExecCount += count;
                                if (curModuleNode.execCount.containsKey(curNode.status)) {
                                    curModuleNode.execCount.put(curNode.status,
                                            new Long(count) + curModuleNode.execCount.get(curNode.status));
                                } else {
                                    curModuleNode.execCount.put(curNode.status, new Long(count));
                                }
                                appNode.totalExecCount += count;
                                if (appNode.execCount.containsKey(curNode.status)) {
                                    appNode.execCount.put(curNode.status,
                                            new Long(count) + appNode.execCount.get(curNode.status));
                                } else {
                                    appNode.execCount.put(curNode.status, new Long(count));
                                }

                                // code coverage
                                if (count > 0) {
                                    curNode.insnExecCount++;
                                    curBlockNode.insnExecCount++;
                                    curFuncNode.insnExecCount++;
                                    curModuleNode.insnExecCount++;
                                    appNode.insnExecCount++;
                                }
                            }
                        }
                    }
                }
            }

        } catch (ParserConfigurationException ex) {
            JOptionPane.showMessageDialog(null, "Parser configuration error: " + ex.getMessage());
            ex.printStackTrace();
        } catch (SAXException ex) {
            JOptionPane.showMessageDialog(null, "XML parsing error: " + ex.getMessage());
            ex.printStackTrace();
        } catch (IOException ex) {
            JOptionPane.showMessageDialog(null, "I/O error: " + ex.getMessage());
            ex.printStackTrace();
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, "Error: " + ex.getMessage());
            ex.printStackTrace();
        }
    }

    public void refreshKeyLabels() {
        int noneCount      = 0;
        int ignoreCount    = 0;
        int singleCount    = 0;
        int doubleCount    = 0;
        int candidateCount = 0;
        ConfigTreeNode.CNStatus status;
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        ConfigTreeNode curModNode = null;
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;

        Enumeration<ConfigTreeNode> modules = appNode.children();
        while (modules.hasMoreElements()) {
            curModNode = modules.nextElement();
            Enumeration<ConfigTreeNode> funcs = curModNode.children();
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
                            case CANDIDATE: candidateCount++;      break;
                            default:                            break;
                        }
                    }
                }
            }
        }
        noneKeyLabel.setText(" NONE (" + noneCount + ") ");
        ignoreKeyLabel.setText(" IGNORE (" + ignoreCount + ") ");
        singleKeyLabel.setText(" SINGLE (" + singleCount + ") ");
        doubleKeyLabel.setText(" DOUBLE (" + doubleCount + ") ");
        candidateKeyLabel.setText(" CANDIDATE (" + candidateCount + ") ");
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
        ConfigTreeNode curModNode = null;
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;
        Enumeration<ConfigTreeNode> modules = appNode.children();
        while (modules.hasMoreElements()) {
            curModNode = modules.nextElement();
            Enumeration<ConfigTreeNode> funcs = curModNode.children();
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
        }
        mainTree.repaint();
        refreshKeyLabels();
    }

    public void removeNonExecutedEntries() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;

        Set<ConfigTreeNode> nodesToRemove = new HashSet<ConfigTreeNode>();

        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        ConfigTreeNode curModNode = null;
        ConfigTreeNode curFuncNode = null;
        ConfigTreeNode curBlockNode = null;
        ConfigTreeNode curNode = null;
        Enumeration<ConfigTreeNode> modules = appNode.children();
        while (modules.hasMoreElements()) {
            curModNode = modules.nextElement();
            if (curModNode.totalExecCount == 0) {
                nodesToRemove.add(curModNode);
            } else {
                Enumeration<ConfigTreeNode> funcs = curModNode.children();
                while (funcs.hasMoreElements()) {
                    curFuncNode = funcs.nextElement();
                    if (curFuncNode.totalExecCount == 0) {
                        nodesToRemove.add(curFuncNode);
                    } else {
                        Enumeration<ConfigTreeNode> blocks = curFuncNode.children();
                        while (blocks.hasMoreElements()) {
                            curBlockNode = blocks.nextElement();
                            if (curBlockNode.totalExecCount == 0) {
                                nodesToRemove.add(curBlockNode);
                            } else {
                                Enumeration<ConfigTreeNode> insns = curBlockNode.children();
                                while (insns.hasMoreElements()) {
                                    curNode = insns.nextElement();
                                    if (curNode.totalExecCount == 0) {
                                        nodesToRemove.add(curNode);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        for (ConfigTreeNode node : nodesToRemove) {
            TreeNode parent = node.getParent();
            if (parent instanceof ConfigTreeNode) {
                ((ConfigTreeNode)parent).remove(node);
            }
        }

        refreshTree(appNode);
    }

    public void expandAllRows() {
        for (int i = 0; i < mainTree.getRowCount(); i++) {
            mainTree.expandRow(i);
        }
    }

    public void expandRows(ConfigTreeNode.CNStatus status) {
        for (int i = 0; i < mainTree.getRowCount(); i++) {
            ConfigTreeNode curNode = (ConfigTreeNode)mainTree.getPathForRow(i).getLastPathComponent();
            if ((curNode.type == ConfigTreeNode.CNType.MODULE ||
                 curNode.type == ConfigTreeNode.CNType.FUNCTION ||
                 curNode.type == ConfigTreeNode.CNType.BASIC_BLOCK) &&
                    curNode.status == ConfigTreeNode.CNStatus.NONE &&
                    shouldExpandRow(curNode, status)) {
                mainTree.expandRow(i);
            } else if (curNode.type != ConfigTreeNode.CNType.APPLICATION) {
                mainTree.collapseRow(i);
            }
        }
    }

    public boolean shouldExpandRow(TreeNode parent, ConfigTreeNode.CNStatus status) {
        if (parent == null || !(parent instanceof ConfigTreeNode)) return false;
        ConfigTreeNode child = null;
        Enumeration<ConfigTreeNode> children = parent.children();
        while (children.hasMoreElements()) {
            child = children.nextElement();
            if (child.status == status ||
                    shouldExpandRow(child, status)) {
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

    public void setSelection(ConfigTreeNode.CNStatus newStatus) {
        TreePath[] paths = mainTree.getSelectionPaths();
        if (paths != null) {
            for (int i = 0; i < paths.length; i++) {
                TreePath selPath = paths[i];
                Object obj = selPath.getLastPathComponent();
                if (obj instanceof ConfigTreeNode) {
                    ((ConfigTreeNode)obj).status = newStatus;
                }
            }
            mainTree.repaint();
            refreshKeyLabels();
        }
    }

    public void openSourceWindow(ConfigTreeNode node) {
        // can't open anything except instructions
        if (node.type != ConfigTreeNode.CNType.INSTRUCTION) return;

        // extract filename and line number from the label
        String filename = "", linenoStr = "0";
        Pattern p = Pattern.compile("\\[([^\\[:]*):(\\d+)\\]");
        Matcher m = p.matcher(node.label);
        if (m.find()) {
            filename = m.group(1);
            linenoStr = m.group(2);
        }
        int lineno = 0;
        try {
            lineno = Integer.parseInt(linenoStr);
        } catch (NumberFormatException e) {};

        // find the source file
        String fullPath = null;
        for (File path : Util.getSearchDirs()) {
            if (fullPath == null) {
                fullPath = Util.findFile(path, filename);
                if (fullPath != null) {
                    File parent = (new File(fullPath)).getParentFile();
                    if (parent != null) {
                        Util.addSearchDir(parent);
                    }
                    break;
                }
            }
        }

        // build the text area
        JTextArea sourceCode = new JTextArea();
        sourceCode.setFont(DEFAULT_FONT_MONO_PLAIN);
        sourceCode.setBorder(new LineNumberedBorder(
                    LineNumberedBorder.LEFT_SIDE, LineNumberedBorder.LEFT_JUSTIFY));
        JScrollPane sourcePanel = new JScrollPane(sourceCode);

        // load the file
        if (fullPath != null) {
            try {
                BufferedReader reader = new BufferedReader(new FileReader(fullPath));
                StringBuffer code = new StringBuffer();
                String nextLine = reader.readLine();
                int lineCount = 1;
                int selStart = 0, selStop = 0;
                while (nextLine != null) {
                    if (lineCount == lineno)
                        selStart = code.length();
                    code.append(nextLine);
                    if (lineCount == lineno)
                        selStop = code.length();
                    code.append(System.getProperty("line.separator"));
                    nextLine = reader.readLine();
                    lineCount++;
                }
                sourceCode.setText(code.toString());
                sourceCode.select(selStart, selStop);
            } catch (Exception ex) {
                sourceCode.setText(ex.getMessage());
            }
        } else {
            sourceCode.setText("Cannot find file: " + filename);
        }

        // build the window
        JFrame sourceWindow = new JFrame("Source Viewer: " + filename);
        sourceWindow.getContentPane().add(sourcePanel);
        sourceWindow.setSize(800, 500);
        sourceWindow.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        sourceWindow.setLocationRelativeTo(this);
        sourceWindow.setVisible(true);
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
        } else if (e.getSource() == expandSingleButton) {
            expandRows(ConfigTreeNode.CNStatus.SINGLE);
        } else if (e.getSource() == expandDoubleButton) {
            expandRows(ConfigTreeNode.CNStatus.DOUBLE);
        } else if (e.getSource() == collapseAllButton) {
            collapseAllRows();
        } else if (e.getSource() == showEffectiveBox) {
            mainRenderer.setShowEffectiveStatus(showEffectiveBox.isSelected());
            mainTree.repaint();
            refreshKeyLabels();
        } else if (e.getSource() == toggleButton) {
            toggleSelection();
        } else if (e.getSource() == setNoneButton) {
            setSelection(ConfigTreeNode.CNStatus.NONE);
        } else if (e.getSource() == setIgnoreButton) {
            setSelection(ConfigTreeNode.CNStatus.IGNORE);
        } else if (e.getSource() == setSingleButton) {
            setSelection(ConfigTreeNode.CNStatus.SINGLE);
        } else if (e.getSource() == setDoubleButton) {
            setSelection(ConfigTreeNode.CNStatus.DOUBLE);
        } else if (e.getSource() == setCandidateButton) {
            setSelection(ConfigTreeNode.CNStatus.CANDIDATE);
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

        // main application object
        ConfigEditorApp app = null;

        // parse command-line args
        for (i = 0; i < args.length; i++) {
            files.add(new File(args[i]));
        }

        // open all given log files
        for (File f : files) {
            if (app == null) {
                app = new ConfigEditorApp();
                app.openFile(f);
            } else {
                if (Util.getExtension(f).equals("cfg")) {
                    app.mergeConfigFile(f);
                } else if (Util.getExtension(f).equals("log")) {
                    app.mergeLogFile(f);
                } else {
                    JOptionPane.showMessageDialog(null, "Invalid file extension: " + f.getName());
                }
            }
        }

        // empty config if no file is given
        if (app == null) {
            app = new ConfigEditorApp();
        }

        app.setVisible(true);

    } // }}}

}

