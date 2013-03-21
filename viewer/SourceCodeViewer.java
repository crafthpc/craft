/**
 * SourceCodeViewer
 *
 * This viewer supports opening source code files. It uses a custom text box for
 * line numbers, and it can optionally display a list of files for the user to
 * switch between.
 */

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.text.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.*;
import javax.swing.event.*;

public class SourceCodeViewer extends JFrame implements ListSelectionListener {

    public static final String REGEX = "\\[([^\\[:]*):(\\d+)\\]";

    public JTextArea sourceCode = new JTextArea();
    public JList sourceFiles = new JList();
    public JSplitPane mainPanel;
    public String currentFile = "";

    public SourceCodeViewer(ConfigTreeNode node) {
        this(node, null);
    }

    public SourceCodeViewer(ConfigTreeNode node, ConfigTreeNode root) {
        currentFile = getFilename(node);
        int lineNumber = getLineNumber(node);
        initialize();
        if (root != null) {
            addConfigInfo(root);
        }
        loadFile(currentFile, lineNumber);
    }

    public SourceCodeViewer(String filename) {
        this(filename, -1, null);
    }

    public SourceCodeViewer(String filename, int lineNumber) {
        this(filename, lineNumber, null);
    }
    
    public SourceCodeViewer(String filename, int lineNumber, ConfigTreeNode root) {
        currentFile = filename;
        initialize();
        if (root != null) {
            addConfigInfo(root);
        }
        loadFile(currentFile, lineNumber);
    }

    public void initialize() {

        // build file list
        sourceFiles = new JList();
        sourceFiles.setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        sourceFiles.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        sourceFiles.getSelectionModel().addListSelectionListener(this);
        JScrollPane sourceFilePanel = new JScrollPane(sourceFiles);

        // build main source code textbox
        sourceCode = new JTextArea();
        sourceCode.setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        sourceCode.setBorder(new LineNumberedBorder(
                    LineNumberedBorder.LEFT_SIDE, LineNumberedBorder.LEFT_JUSTIFY));
        JScrollPane sourcePanel = new JScrollPane(sourceCode);

        // build main splitpane layout
        mainPanel = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true,
                sourceFilePanel, sourcePanel);

        // set up window
        getContentPane().add(mainPanel);
        setSize(1000, 800);
        setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        setLocationRelativeTo(this);
    }

    public void loadFile(String filename) {
        loadFile(filename, -1);
    }

    public void loadFile(String filename, int lineNumber) {

        currentFile = filename;
        setTitle("Source Viewer: " + filename);

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

        // load the file
        if (fullPath != null) {
            try {
                BufferedReader reader = new BufferedReader(new FileReader(fullPath));
                StringBuffer code = new StringBuffer();
                String nextLine = reader.readLine();
                int lineCount = 1;
                int selStart = 0, selStop = 0;
                while (nextLine != null) {
                    if (lineCount == lineNumber)
                        selStart = code.length();
                    code.append(nextLine);
                    if (lineCount == lineNumber)
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
    }

    public void fileAllFiles(ConfigTreeNode node, Set<String> filenames) {

        if (node.type == ConfigTreeNode.CNType.INSTRUCTION) {

            // it's an instruction; check for filename
            String fn = getFilename(node);
            if (!fn.equals("")) {
                filenames.add(fn);
            }
        } else {

            // it's not an instruction; recurse on its child nodes
            ConfigTreeNode child = null;
            Enumeration<ConfigTreeNode> children = node.children();
            while (children.hasMoreElements()) {
                child = children.nextElement();
                fileAllFiles(child, filenames);
            }
        }
    }

    public void addConfigInfo(ConfigTreeNode root) {

        // recursively find all the files in this config tree, and add them to
        // the file list box
        SortedSet<String> filenames = new TreeSet<String>();
        fileAllFiles(root, filenames);
        setFileList(filenames);
    }

    public void setFileList(Set<String> filenames) {

        // add unique filenames to the file list
        // also, make sure the currently-open one is selected
        // pass in a SortedSet if you want the files listed in order
        int selectedFile = -1;
        DefaultListModel model = new DefaultListModel();
        for (String fn : filenames) {
            if (fn.equals(currentFile)) {
                selectedFile = model.size();
            }
            model.add(model.size(), fn);
        }
        sourceFiles.setModel(model);
        if (selectedFile >= 0) {
            sourceFiles.setSelectedIndex(selectedFile);
        }
    }

    public String getFilename(ConfigTreeNode node) {

        // extract the filename from the node's label
        String filename = "";
        Pattern p = Pattern.compile(REGEX);
        Matcher m = p.matcher(node.label);
        if (m.find()) {
            filename = m.group(1);
        }
        return filename;
    }

    public int getLineNumber(ConfigTreeNode node) {

        // extract the line number from the node's label
        String linenoStr = "0";
        Pattern p = Pattern.compile(REGEX);
        Matcher m = p.matcher(node.label);
        if (m.find()) {
            linenoStr = m.group(2);
        }
        int lineno = 0;
        try {
            lineno = Integer.parseInt(linenoStr);
        } catch (NumberFormatException e) {};
        return lineno;
    }

    public void refreshInterface() {

        // resize the splitter
        if (sourceFiles.getModel().getSize() > 1) {
            mainPanel.setDividerLocation(0.2);
        } else {
            mainPanel.setDividerLocation(0.0);
        }

        // focus the code box (so that any selection is visible)
        sourceCode.requestFocusInWindow();
    }

    public void valueChanged(ListSelectionEvent e) {
        Object obj = sourceFiles.getSelectedValue();
        if (obj == null) return;
        if (!(obj instanceof String)) return;
        String fn = (String)obj;
        loadFile(fn);
    }
}

