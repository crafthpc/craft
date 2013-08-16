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
    public static final Color DEFAULT_COLOR_IGNORE    = new Color(240, 250, 160); // (225, 225,   0);
    public static final Color DEFAULT_COLOR_CANDIDATE = new Color(105, 210, 231); // (125, 175, 225);
    public static final Color DEFAULT_COLOR_SINGLE    = new Color(209, 242, 165); // (125, 255, 125);
    public static final Color DEFAULT_COLOR_DOUBLE    = new Color(252, 157, 154); // (255, 120, 120);
    public static final Color DEFAULT_COLOR_RPREC     = new Color(255, 212, 120);
    public static final Color DEFAULT_COLOR_NULL      = new Color(224, 224, 224);
    public static final Color DEFAULT_COLOR_TRANGE    = new Color(244, 182, 252);
    public static final Color DEFAULT_COLOR_CINST     = new Color(244, 182, 252);
    public static final Color DEFAULT_COLOR_DCANCEL   = new Color(244, 182, 252);
    public static final Color DEFAULT_COLOR_DNAN      = new Color(244, 182, 252);
    public static final Color DEFAULT_COLOR_BORDER    = new Color(  0,   0, 225);

    // app-wide variables
    private static String fpconfOptions = "";

    // main data structures
    public File mainConfigFile;
    public java.util.List<ConfigTreeNode> mainConfigEntries;
    public java.util.List<String> mainConfigMiscEntries;
    public Map<String, ConfigTreeNode> nodeOfID;

    // main interface elements
    public JFileChooser fileChooser;
    public JFileChooser fileAllChooser;
    public JPanel mainPanel;
    public JPanel topPanel;
    public JLabel filenameLabel;
    public JTextField searchBox;
    public JButton expandAllButton;
    public JButton expandSingleButton;
    public JButton expandDoubleButton;
    public JButton expandTestedButton;
    public JButton collapseAllButton;
    public JCheckBox showEffectiveBox;
    public JCheckBox showCodeCoverageBox;
    public JCheckBox showErrorBox;
    public JCheckBoxMenuItem showIDMenu;
    public JCheckBoxMenuItem showEffectiveMenu;
    public JCheckBoxMenuItem showCodeCoverageMenu;
    public JCheckBoxMenuItem showErrorMenu;
    public JCheckBoxMenuItem showPrecisionMenu;
    public JButton toggleButton;
    public JButton setNoneButton;
    public JButton setIgnoreButton;
    public JButton setCandidateButton;
    public JButton setSingleButton;
    public JButton setDoubleButton;
    public JButton setNullButton;
    public JButton sourceButton;
    public JButton saveButton;
    public JTree mainTree;
    public ConfigTreeRenderer mainRenderer;
    public JLabel noneKeyLabel;
    public JLabel ignoreKeyLabel;
    public JLabel candidateKeyLabel;
    public JLabel singleKeyLabel;
    public JLabel doubleKeyLabel;
    public JLabel rprecKeyLabel;
    public JLabel nullKeyLabel;
    public JLabel miscKeyLabel;
    public JPanel bottomPanel;

    // }}}
    
    // {{{ general initialization

    public ConfigEditorApp() {
        super("");

        fileChooser = new JFileChooser();
        fileChooser.setCurrentDirectory(new File("."));
        fileChooser.setFileFilter(new ConfigFilter());
        fileAllChooser = new JFileChooser();
        fileAllChooser.setCurrentDirectory(new File("."));
        fileAllChooser.setFileFilter(new ConfigAllFilter());

        Util.initSearchDirs();

        buildMenuBar();
        buildMainInterface();
    }

    // }}}
    // {{{ initialize main interface

    public void buildMenuBar() {
        JMenu fileMenu = new JMenu("File");
        fileMenu.setMnemonic(KeyEvent.VK_F);
        fileMenu.add(new OpenConfigAction (this, "Open",  null, Integer.valueOf(KeyEvent.VK_O)));
        fileMenu.add(new MergeConfigAction(this, "Merge", null, Integer.valueOf(KeyEvent.VK_R)));
        fileMenu.add(new SaveConfigAction (this, "Save",  null, Integer.valueOf(KeyEvent.VK_S)));

        JMenu actionMenu = new JMenu("Actions");
        actionMenu.setMnemonic(KeyEvent.VK_A);
        actionMenu.add(new BatchConfigAction(this, "Batch Config", null, Integer.valueOf(KeyEvent.VK_B)));
        actionMenu.add(new JSeparator());
        actionMenu.add(new RemoveNonExecutedAction (this, "Remove Non-executed Entries",  null, Integer.valueOf(KeyEvent.VK_N)));
        actionMenu.add(new RemoveMovementAction    (this, "Remove Movement Entries",      null, Integer.valueOf(KeyEvent.VK_M)));
        actionMenu.add(new RemoveNonCandidateAction(this, "Remove Non-candidate Entries", null, Integer.valueOf(KeyEvent.VK_C)));

        JMenu viewMenu = new JMenu("View");
        viewMenu.setMnemonic(KeyEvent.VK_V);
        showIDMenu = new JCheckBoxMenuItem("View ID");
        showIDMenu.addActionListener(this);
        viewMenu.add(showIDMenu);
        showEffectiveMenu = new JCheckBoxMenuItem("View Effective Status");
        showEffectiveMenu.addActionListener(this);
        viewMenu.add(showEffectiveMenu);
        showCodeCoverageMenu = new JCheckBoxMenuItem("View Code Coverage");
        showCodeCoverageMenu.addActionListener(this);
        viewMenu.add(showCodeCoverageMenu);
        showErrorMenu = new JCheckBoxMenuItem("View Reported Error");
        showErrorMenu.addActionListener(this);
        viewMenu.add(showErrorMenu);
        showPrecisionMenu = new JCheckBoxMenuItem("View Reduced Precision Levels");
        showPrecisionMenu.addActionListener(this);
        viewMenu.add(showPrecisionMenu);
        viewMenu.add(new JSeparator());
        viewMenu.add(new ExpandRowsAction(this, "Expand All",     null, null, "all"));
        viewMenu.add(new ExpandRowsAction(this, "Collapse All",   null, null, "none"));
        viewMenu.add(new ExpandRowsAction(this, "Expand Singles", null, null, "single"));
        viewMenu.add(new ExpandRowsAction(this, "Expand Doubles", null, null, "double"));
        viewMenu.add(new ExpandRowsAction(this, "Expand Ignore",  null, null, "ignore"));
        viewMenu.add(new ExpandRowsAction(this, "Expand Tested",  null, null, "tested"));

        JMenu reportMenu = new JMenu("Reports");
        reportMenu.setMnemonic(KeyEvent.VK_R);
        reportMenu.add(new ConfigReportAction(this, new RPrecConfigReport(), 
                    "Reduced-precision Report", null, null));

        JMenuBar menuBar = new JMenuBar();
        menuBar.add(fileMenu);
        menuBar.add(viewMenu);
        menuBar.add(reportMenu);
        menuBar.add(actionMenu);
        setJMenuBar(menuBar);
    }

    public void buildMainInterface() {

        topPanel = new JPanel();
        filenameLabel = new JLabel("");
        //topPanel.add(filenameLabel);  // kind of redundant; it's in the window title
        topPanel.add(new JLabel("   "));
        topPanel.add(new JLabel("Search:"));
        searchBox = new JTextField(20);
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
        //topPanel.add(expandSingleButton);
        expandDoubleButton = new JButton("View doubles");
        expandDoubleButton.addActionListener(this);
        //topPanel.add(expandDoubleButton);
        expandTestedButton = new JButton("View tested");
        expandTestedButton.addActionListener(this);
        topPanel.add(expandTestedButton);
        topPanel.add(new JLabel("   "));
        topPanel.add(new JLabel("   "));
        showEffectiveBox = new JCheckBox("Show effective status");
        showEffectiveBox.addActionListener(this);
        //topPanel.add(showEffectiveBox);
        showCodeCoverageBox = new JCheckBox("Show code coverage");
        showCodeCoverageBox.addActionListener(this);
        topPanel.add(showCodeCoverageBox);
        showErrorBox = new JCheckBox("Show error");
        showErrorBox.addActionListener(this);
        //topPanel.add(showErrorBox);
        topPanel.add(new JLabel("   "));
        sourceButton = new JButton("Show Source");
        sourceButton.addActionListener(this);
        topPanel.add(sourceButton);
        saveButton = new JButton("Save");
        saveButton.addActionListener(this);
        topPanel.add(saveButton);

        mainTree = new JTree();
        mainRenderer = new ConfigTreeRenderer();
        showIDMenu.setSelected(mainRenderer.getShowID());
        showEffectiveBox.setSelected(mainRenderer.getShowEffectiveStatus());
        showEffectiveMenu.setSelected(mainRenderer.getShowEffectiveStatus());
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
        bottomPanel.add(new JLabel(" "));
        ignoreKeyLabel = new JLabel("IGNORE");
        ignoreKeyLabel.setBackground(DEFAULT_COLOR_IGNORE);
        ignoreKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        ignoreKeyLabel.setOpaque(true);
        bottomPanel.add(ignoreKeyLabel);
        bottomPanel.add(new JLabel(" "));
        candidateKeyLabel = new JLabel("CANDIDATE");
        candidateKeyLabel.setBackground(DEFAULT_COLOR_CANDIDATE);
        candidateKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        candidateKeyLabel.setOpaque(true);
        bottomPanel.add(candidateKeyLabel);
        bottomPanel.add(new JLabel(" "));
        singleKeyLabel = new JLabel("SINGLE");
        singleKeyLabel.setBackground(DEFAULT_COLOR_SINGLE);
        singleKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        singleKeyLabel.setOpaque(true);
        bottomPanel.add(singleKeyLabel);
        bottomPanel.add(new JLabel(" "));
        doubleKeyLabel = new JLabel("DOUBLE");
        doubleKeyLabel.setBackground(DEFAULT_COLOR_DOUBLE);
        doubleKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        doubleKeyLabel.setOpaque(true);
        bottomPanel.add(doubleKeyLabel);
        bottomPanel.add(new JLabel(" "));
        rprecKeyLabel = new JLabel("RPREC");
        rprecKeyLabel.setBackground(DEFAULT_COLOR_RPREC);
        rprecKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        rprecKeyLabel.setOpaque(true);
        bottomPanel.add(rprecKeyLabel);
        bottomPanel.add(new JLabel(" "));
        nullKeyLabel = new JLabel("NULL");
        nullKeyLabel.setBackground(DEFAULT_COLOR_NULL);
        nullKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        nullKeyLabel.setOpaque(true);
        bottomPanel.add(nullKeyLabel);
        bottomPanel.add(new JLabel(" "));
        miscKeyLabel = new JLabel("MISC");
        miscKeyLabel.setBackground(DEFAULT_COLOR_TRANGE);
        miscKeyLabel.setFont(DEFAULT_FONT_MONO_BOLD);
        miscKeyLabel.setOpaque(true);
        bottomPanel.add(miscKeyLabel);
        refreshKeyLabels();

        bottomPanel.add(new JLabel("      "));
        setNoneButton = new JButton(" ");
        setNoneButton.setBackground(Color.WHITE);
        setNoneButton.addActionListener(this);
        bottomPanel.add(setNoneButton);
        setIgnoreButton = new JButton("!");
        setIgnoreButton.setBackground(DEFAULT_COLOR_IGNORE);
        setIgnoreButton.addActionListener(this);
        bottomPanel.add(setIgnoreButton);
        setCandidateButton = new JButton("?");
        setCandidateButton.setBackground(DEFAULT_COLOR_CANDIDATE);
        setCandidateButton.addActionListener(this);
        bottomPanel.add(setCandidateButton);
        setSingleButton = new JButton("S");
        setSingleButton.setBackground(DEFAULT_COLOR_SINGLE);
        setSingleButton.addActionListener(this);
        bottomPanel.add(setSingleButton);
        setDoubleButton = new JButton("D");
        setDoubleButton.setBackground(DEFAULT_COLOR_DOUBLE);
        setDoubleButton.addActionListener(this);
        bottomPanel.add(setDoubleButton);
        setNullButton = new JButton("X");
        setNullButton.setBackground(DEFAULT_COLOR_NULL);
        setNullButton.addActionListener(this);
        bottomPanel.add(setNullButton);
        toggleButton = new JButton("...");
        toggleButton.setBackground(DEFAULT_COLOR_TRANGE);
        toggleButton.addActionListener(this);
        //bottomPanel.add(toggleButton);

        mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(new JScrollPane(mainTree), BorderLayout.CENTER);
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);

        getContentPane().add(mainPanel);
        setSize(1400,900);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);
        setTitle(DEFAULT_TITLE);

        mainPanel.revalidate();
    }

    // }}}

    // {{{ helper/utility functions

    private static final String RPREC_REGEX = "(\\w+_\\d+)_precision=(\\d+)";

    private void parseReducedPrecisionConfig(String line) {
        // TODO: generalize to any type of precision config line
        String id = Util.extractRegex(line, RPREC_REGEX, 1);
        if (nodeOfID != null && id != null) {
            if (nodeOfID.containsKey(id)) {
                ConfigTreeNode node = nodeOfID.get(id);
                String prec = Util.extractRegex(line, RPREC_REGEX, 2);
                node.precision = Long.parseLong(prec);
                setShowPrecision(true);
            }
        }
    }

    public void openFile(File file) {
        if (file.getName().endsWith(".cfg")) {

            // config file; open as usual
            openConfigFile(file);

        } else {

            // this is a binary; use a config file in the current directory
            // with the same base name
            File cfgFile = new File(file.getName() + ".cfg");
            String cmd = FPCONF_PATH + " " + fpconfOptions + " " + file.getAbsolutePath();
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
        nodeOfID = new HashMap<String, ConfigTreeNode>();

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
                        nodeOfID.put(ConfigTreeNode.type2Str(curNode.type) + 
                                "_" + curNode.number, curNode);
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
                        nodeOfID.put(ConfigTreeNode.type2Str(curNode.type) + 
                                "_" + curNode.number, curNode);
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
                        nodeOfID.put(ConfigTreeNode.type2Str(curNode.type) + 
                                "_" + curNode.number, curNode);
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
                        nodeOfID.put(ConfigTreeNode.type2Str(curNode.type) + 
                                "_" + curNode.number, curNode);
                    } else if (curNode.type == ConfigTreeNode.CNType.INSTRUCTION &&
                               curBlockNode != null) {
                        curBlockNode.add(curNode);
                        mainConfigEntries.add(curNode);
                        nodeOfID.put(ConfigTreeNode.type2Str(curNode.type) + 
                                "_" + curNode.number, curNode);
                    }
                } else {
                    mainConfigMiscEntries.add(curLine);
                }
            }

            rdr.close();
        } catch (IOException e) {
            System.err.println("ERROR: " + e.getMessage());
        }

        for (String s : mainConfigMiscEntries) {
            parseReducedPrecisionConfig(s);
        }
        appNode.updatePrecision();

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

    public void mergeConfigFile(File file) {

        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
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
        StringBuffer errors = new StringBuffer();

        try {
            BufferedReader rdr = new BufferedReader(new FileReader(file));

            // scan the file and build the configuration structure;
            // this works by reading through the file and simultaneously
            // traversing the existing config tree;
            // this assumes the files have the same structure and ordering,
            // which should be the case
            //
            while ((curLine = rdr.readLine()) != null) {
                if (curLine.startsWith("^")) {
                    curNode = new ConfigTreeNode(curLine);
                    if (curNode.type == ConfigTreeNode.CNType.APPLICATION) {
                        appNode = curNode;
                        if (!appNode.label.equals(origAppNode.label)) {
                            success = false;
                            errors.append("Mismatched labels for " + curNode.label + "\n");
                        } else {
                            origAppNode.merge(appNode);
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.MODULE) {
                        curModNode = curNode;
                        Enumeration origModules = origAppNode.children();
                        boolean found = false;
                        while (origModules.hasMoreElements()) {
                            ConfigTreeNode mod = (ConfigTreeNode)origModules.nextElement();
                            if (mod.label.equals(curModNode.label)) {
                                mod.merge(curModNode);
                                origModNode = mod;
                                found = true;
                            }
                        }
                        if (!found) {
                            success = false;
                            errors.append("Cannot find original entry for " + curNode.label + "\n");
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.FUNCTION &&
                               curModNode != null && origModNode != null) {
                        curFuncNode = curNode;
                        Enumeration origFuncs = origModNode.children();
                        boolean found = false;
                        while (origFuncs.hasMoreElements()) {
                            ConfigTreeNode func = (ConfigTreeNode)origFuncs.nextElement();
                            if (func.label.equals(curFuncNode.label)) {
                                func.merge(curFuncNode);
                                origFuncNode = func;
                                found = true;
                            }
                        }
                        if (!found) {
                            success = false;
                            errors.append("Cannot find original entry for " + curNode.label + "\n");
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.BASIC_BLOCK &&
                               curFuncNode != null && origFuncNode != null) {
                        curBlockNode = curNode;
                        Enumeration origBlocks = origFuncNode.children();
                        boolean found = false;
                        while (origBlocks.hasMoreElements()) {
                            ConfigTreeNode block = (ConfigTreeNode)origBlocks.nextElement();
                            if (block.label.equals(curBlockNode.label)) {
                                block.merge(curBlockNode);
                                origBlockNode = block;
                                found = true;
                            }
                        }
                        if (!found) {
                            success = false;
                            errors.append("Cannot find original entry for " + curNode.label + "\n");
                        }

                    } else if (curNode.type == ConfigTreeNode.CNType.INSTRUCTION &&
                               curBlockNode != null && origBlockNode != null) {
                        Enumeration origInsns = origBlockNode.children();
                        boolean found = false;
                        while (origInsns.hasMoreElements()) {
                            ConfigTreeNode insn = (ConfigTreeNode)origInsns.nextElement();
                            if (insn.label.equals(curNode.label)) {
                                insn.merge(curNode);
                                found = true;
                            }
                        }
                        if (!found) {
                            success = false;
                            errors.append("Cannot find original entry for " + curNode.label + "\n");
                        }
                    }
                } else {
                    // TODO: do something with misc config entries?
                    // we don't want to just add them in because they might
                    // conflict with existing entries
                    //mainConfigMiscEntries.add(curLine);
                }
            }

            rdr.close();
        } catch (IOException e) {
            System.err.println("ERROR: " + e.getMessage());
        }

        if (!success) {
            String message = "Errors merging file " + file.getName() + ":\n\n" + errors.toString();
            //System.out.println(message);
            //JOptionPane.showMessageDialog(null, message);
        }

        setTitle(DEFAULT_TITLE + " - (multiple files)");
        filenameLabel.setText("(multiple files)");          // saveConfigFile() depends on this string
        mainTree.repaint();
        refreshKeyLabels();
    }
    
    public void saveConfigFile() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;

        // make sure the user is aware if they are saving multiple merged files
        // to a single file
        if (filenameLabel.getText().equals("(multiple files)")) {
            int rval = JOptionPane.showConfirmDialog(this,
                    "Multiple files are open, but this will save the merged contents to \""
                    + mainConfigFile.getName() + "\". Overwrite it?",
                    DEFAULT_TITLE, JOptionPane.YES_NO_OPTION);
            if (rval == JOptionPane.NO_OPTION) {
                return;
            }
        }

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

    public void recalculateExecutionCounts(LLogFile logfile) {
        // walk the config tree and retrieve/calculate all execution counts
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        appNode.updateExecCounts(logfile);
    }

    public void addTestedResults(TestedResultFile rfile) {

        // walk the config tree and save all regexTags into a lookup table
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        Map<String,ConfigTreeNode> lookup = new HashMap<String,ConfigTreeNode>();
        appNode.getRegexTagLookups(lookup);

        // iterate over the results and add them to the tree
        for (TestedResult r : rfile.allResults) {
            for (String k : r.exceptions.keySet()) {
                if (lookup.containsKey(k)) {
                    ConfigTreeNode node = lookup.get(k);
                    node.setTested(true);
                    node.setError(r.error);
                }
            }
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
            recalculateExecutionCounts(logfile);
            setShowCodeCoverage(true);
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
        mainTree.repaint();
        refreshKeyLabels();
    }

    public void mergeTestedFile(File file) {
        if (!file.exists()) {
            JOptionPane.showMessageDialog(null, "I/O error: " + file.getAbsolutePath() + " does not exist!");
            return;
        }
        TestedResultFile rfile = new TestedResultFile(file);
        addTestedResults(rfile);
        setShowError(true);
    }

    public void refreshTreeLabels() {
        for (int i = 0; i < mainTree.getRowCount(); i++) {
            ConfigTreeNode curNode = (ConfigTreeNode)mainTree.getPathForRow(i).getLastPathComponent();
            ((DefaultTreeModel)mainTree.getModel()).nodeChanged(curNode);
        }
        mainTree.repaint();
    }

    public void refreshKeyLabels() {
        Map<ConfigTreeNode.CNStatus,Long> allCounts = new HashMap<ConfigTreeNode.CNStatus,Long>();
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        appNode.getStatusCounts(allCounts);
        long noneCount      = 0;
        long ignoreCount    = 0;
        long candidateCount = 0;
        long singleCount    = 0;
        long doubleCount    = 0;
        long rprecCount     = 0;
        long nullCount      = 0;
        long miscCount      = 0;
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.NONE)) {
            noneCount      += (allCounts.get(ConfigTreeNode.CNStatus.NONE).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.IGNORE)) {
            ignoreCount    += (allCounts.get(ConfigTreeNode.CNStatus.IGNORE).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.CANDIDATE)) {
            candidateCount += (allCounts.get(ConfigTreeNode.CNStatus.CANDIDATE).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.SINGLE)) {
            singleCount    += (allCounts.get(ConfigTreeNode.CNStatus.SINGLE).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.DOUBLE)) {
            doubleCount    += (allCounts.get(ConfigTreeNode.CNStatus.DOUBLE).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.RPREC)) {
            rprecCount     += (allCounts.get(ConfigTreeNode.CNStatus.RPREC).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.NULL)) {
            nullCount      += (allCounts.get(ConfigTreeNode.CNStatus.NULL).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.TRANGE)) {
            miscCount      += (allCounts.get(ConfigTreeNode.CNStatus.TRANGE).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.CINST)) {
            miscCount      += (allCounts.get(ConfigTreeNode.CNStatus.CINST).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.DCANCEL)) {
            miscCount      += (allCounts.get(ConfigTreeNode.CNStatus.DCANCEL).longValue());
        }
        if (allCounts.containsKey(ConfigTreeNode.CNStatus.DNAN)) {
            miscCount      += (allCounts.get(ConfigTreeNode.CNStatus.DNAN).longValue());
        }
        noneKeyLabel.setText     (" NONE ("      + noneCount      + ") ");
        ignoreKeyLabel.setText   (" IGNORE ("    + ignoreCount    + ") ");
        candidateKeyLabel.setText(" CANDIDATE (" + candidateCount + ") ");
        singleKeyLabel.setText   (" SINGLE ("    + singleCount    + ") ");
        doubleKeyLabel.setText   (" DOUBLE ("    + doubleCount    + ") ");
        rprecKeyLabel.setText    (" RPREC ("     + rprecCount     + ") ");
        nullKeyLabel.setText     (" NULL ("      + nullCount      + ") ");
        miscKeyLabel.setText     (" MISC ("      + miscCount      + ") ");
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
        appNode.batchConfig(orig, dest);
        mainTree.repaint();
        refreshKeyLabels();
    }

    public void removeNonExecutedEntries() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        appNode.removeChildren(new ConfigTreeIterator() {
            public boolean test(ConfigTreeNode node) {
                return (node.totalExecCount == 0);
            }
        });
        refreshTree(appNode);
        recalculateExecutionCounts(null);
    }

    public void removeMovementEntries() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        appNode.removeChildren(new ConfigTreeIterator() {
            public boolean test(ConfigTreeNode node) {
                boolean rm = false;
                switch (node.type) {
                    case MODULE:
                    case FUNCTION:
                    case BASIC_BLOCK:
                        rm = (node.getChildCount() == 0);
                        break;
                    case INSTRUCTION:
                        rm = node.label.contains("mov");
                        break;
                    default:
                        break;
                }
                return rm;
            }
        });
        refreshTree(appNode);
        recalculateExecutionCounts(null);
    }

    public void removeNonCandidateEntries() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        appNode.removeChildren(new ConfigTreeIterator() {
            public boolean test(ConfigTreeNode node) {
                boolean rm = false;
                switch (node.type) {
                    case MODULE:
                    case FUNCTION:
                    case BASIC_BLOCK:
                        rm = (node.getChildCount() == 0);
                        break;
                    case INSTRUCTION:
                        rm = !node.candidate;
                        break;
                    default:
                        break;
                }
                return rm;
            }
        });
        refreshTree(appNode);
        recalculateExecutionCounts(null);
    }

    public void removeEmptyNodes() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return;
        ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
        appNode.removeChildren(new ConfigTreeIterator() {
            public boolean test(ConfigTreeNode node) {
                boolean rm = false;
                switch (node.type) {
                    case MODULE:
                    case FUNCTION:
                    case BASIC_BLOCK:
                        rm = (node.getChildCount() == 0);
                        break;
                    default:
                        break;
                }
                return rm;
            }
        });
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

    public void expandTestedRows() {
        for (int i = 0; i < mainTree.getRowCount(); i++) {
            ConfigTreeNode curNode = (ConfigTreeNode)mainTree.getPathForRow(i).getLastPathComponent();
            if (curNode.tested) {
                mainTree.expandRow(i);
            } else if (curNode.type != ConfigTreeNode.CNType.APPLICATION) {
                mainTree.collapseRow(i);
            }
        }
    }

    public boolean shouldExpandRow(TreeNode parent, ConfigTreeNode.CNStatus status) {
        if (parent == null || !(parent instanceof ConfigTreeNode)) return false;
        ConfigTreeNode child = null;
        Enumeration children = parent.children();
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
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

        // make sure there's actually a debug filename in the instruction
        // (faster to check here than to start creating a window and then have
        // to abort it later)
        if (SourceCodeViewer.getFilename(node).equals("")) {
            JOptionPane.showMessageDialog(null, "No source code info for this instruction.");
            return;
        }

        // initialize and show the source code explorer
        SourceCodeViewer sourceWindow;
        if (mainTree.getModel().getRoot() instanceof ConfigTreeNode) {
            ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
            sourceWindow = new SourceCodeViewer(node, appNode);
        } else {
            sourceWindow = new SourceCodeViewer(node);
        }
        sourceWindow.setVisible(true);
        sourceWindow.refreshInterface();
    }

    public void openSourceWindow() {
        SourceCodeViewer sourceWindow = new SourceCodeViewer();
        if (mainTree.getModel().getRoot() instanceof ConfigTreeNode) {
            ConfigTreeNode appNode = (ConfigTreeNode)mainTree.getModel().getRoot();
            sourceWindow.addConfigInfo(appNode);
        }
        sourceWindow.setVisible(true);
        sourceWindow.refreshInterface();
    }

    void setShowID(boolean value) {
        mainRenderer.setShowID(value);
        refreshTreeLabels();
        refreshKeyLabels();
        showIDMenu.setSelected(value);
    }

    void setShowEffectiveStatus(boolean value) {
        mainRenderer.setShowEffectiveStatus(value);
        refreshTreeLabels();
        refreshKeyLabels();
        showEffectiveBox.setSelected(value);
        showEffectiveMenu.setSelected(value);
    }

    void setShowCodeCoverage(boolean value) {
        mainRenderer.setShowCodeCoverage(value);
        refreshTreeLabels();
        refreshKeyLabels();
        showCodeCoverageBox.setSelected(value);
        showCodeCoverageMenu.setSelected(value);
    }

    void setShowError(boolean value) {
        mainRenderer.setShowError(value);
        refreshTreeLabels();
        refreshKeyLabels();
        showErrorBox.setSelected(value);
        showErrorMenu.setSelected(value);
    }

    void setShowPrecision(boolean value) {
        mainRenderer.setShowPrecision(value);
        refreshTreeLabels();
        refreshKeyLabels();
        showPrecisionMenu.setSelected(value);
    }

    ConfigTreeNode getRootNode() {
        if (!(mainTree.getModel().getRoot() instanceof ConfigTreeNode)) return null;
        return (ConfigTreeNode)mainTree.getModel().getRoot();
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
        } else if (e.getSource() == expandTestedButton) {
            expandTestedRows();
        } else if (e.getSource() == collapseAllButton) {
            collapseAllRows();
        } else if (e.getSource() == showIDMenu) {
            setShowID(showIDMenu.isSelected());
        } else if (e.getSource() == showEffectiveBox) {
            setShowEffectiveStatus(showEffectiveBox.isSelected());
        } else if (e.getSource() == showEffectiveMenu) {
            setShowEffectiveStatus(showEffectiveMenu.isSelected());
        } else if (e.getSource() == showCodeCoverageBox) {
            setShowCodeCoverage(showCodeCoverageBox.isSelected());
        } else if (e.getSource() == showCodeCoverageMenu) {
            setShowCodeCoverage(showCodeCoverageMenu.isSelected());
        } else if (e.getSource() == showErrorBox) {
            setShowError(showErrorBox.isSelected());
        } else if (e.getSource() == showErrorMenu) {
            setShowError(showErrorMenu.isSelected());
        } else if (e.getSource() == showPrecisionMenu) {
            setShowPrecision(showPrecisionMenu.isSelected());
        } else if (e.getSource() == toggleButton) {
            toggleSelection();
        } else if (e.getSource() == setNoneButton) {
            setSelection(ConfigTreeNode.CNStatus.NONE);
        } else if (e.getSource() == setIgnoreButton) {
            setSelection(ConfigTreeNode.CNStatus.IGNORE);
        } else if (e.getSource() == setCandidateButton) {
            setSelection(ConfigTreeNode.CNStatus.CANDIDATE);
        } else if (e.getSource() == setSingleButton) {
            setSelection(ConfigTreeNode.CNStatus.SINGLE);
        } else if (e.getSource() == setDoubleButton) {
            setSelection(ConfigTreeNode.CNStatus.DOUBLE);
        } else if (e.getSource() == setNullButton) {
            setSelection(ConfigTreeNode.CNStatus.NULL);
        } else if (e.getSource() == sourceButton) {
            openSourceWindow();
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
        i = 0;
        while (i < args.length) {
            if (args[i].equals("-c") && i < args.length-1) {
                fpconfOptions += args[++i];
            } else {
                File f = new File(args[i]);
                if (f.exists()) {
                    files.add(f);
                } else {
                    JOptionPane.showMessageDialog(null, "File not found: " + f.getName());
                }
            }
            i++;
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
                } else if (Util.getExtension(f).equals("tested")) {
                    app.mergeTestedFile(f);
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

