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

public class SourceCodeViewer extends JFrame
    implements ActionListener, DocumentListener, ListSelectionListener {

    // parsing string for debug info in config node labels
    public static final String REGEX = "\\[([^\\[:]*):(\\d+)\\]";
    public static final Pattern REGEX_PATTERN = Pattern.compile(REGEX);

    // placeholder to use when the source file can't be found
    public static final SourceFileInfo BLANK_FILE_INFO = new SourceFileInfo("");

    // stores source file info (path, replacements, etc.)
    // cached for efficiency (shouldn't need to re-scan config tree
    // every time we switch between files)
    public SortedMap<String, SourceFileInfo> allFiles;

    // which file are we currently viewing?
    // (should be a key into allFiles)
    public String currentFile = "";

    // interface elements
    public JTextArea sourceCode;
    public LineNumberedBorder sourceBorder;
    public JList sourceFiles;
    public JTextField searchBox;
    public JSplitPane mainPanel;


    public SourceCodeViewer() {
        allFiles = new TreeMap<String, SourceFileInfo>();
        initialize();
    }

    public SourceCodeViewer(ConfigTreeNode node) {
        this(node, null);
    }

    public SourceCodeViewer(ConfigTreeNode node, ConfigTreeNode root) {
        currentFile = getFilename(node);
        allFiles = new TreeMap<String, SourceFileInfo>();
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
        allFiles = new TreeMap<String, SourceFileInfo>();
        initialize();
        if (root != null) {
            addConfigInfo(root);
        }
        loadFile(currentFile, lineNumber);
    }

    public void addConfigInfo(ConfigTreeNode root) {

        // recursively find all the files in this config tree, and add them to
        // the file list box; also read any replacement info
        scanConfigInfo(root);
        SourceFileInfo curFileInfo = null;
        refreshFileList();
        if (allFiles.containsKey(currentFile)) {
            sourceBorder.setFileInfo(allFiles.get(currentFile));
        }
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

    private void initialize() {

        // build file list
        sourceFiles = new JList();
        sourceFiles.setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        sourceFiles.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        sourceFiles.getSelectionModel().addListSelectionListener(this);
        JScrollPane sourceFilePanel = new JScrollPane(sourceFiles);

        // build main source code textbox
        sourceCode = new JTextArea();
        sourceCode.setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        sourceBorder = new LineNumberedBorder(LineNumberedBorder.LEFT_SIDE,
                LineNumberedBorder.LEFT_JUSTIFY);
        sourceCode.setBorder(sourceBorder);
        JScrollPane sourcePanel = new JScrollPane(sourceCode);

        // build search box
        searchBox = new JTextField(30);
        searchBox.addActionListener(this);
        searchBox.getDocument().addDocumentListener(this);

        // build right side infrastructure
        JPanel searchPanel = new JPanel();
        searchPanel.add(new JLabel("Search: "));
        searchPanel.add(searchBox);
        JPanel rightPanel = new JPanel();
        rightPanel.setLayout(new BorderLayout());
        rightPanel.add(sourcePanel, BorderLayout.CENTER);
        rightPanel.add(searchPanel, BorderLayout.NORTH);


        // build main splitpane layout
        mainPanel = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true,
                sourceFilePanel, rightPanel);

        // set up window
        getContentPane().add(mainPanel);
        setSize(1200, 800);
        setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        setLocationRelativeTo(this);
    }

    public void loadFile(String filename) {
        loadFile(filename, -1);
    }

    public void loadFile(String filename, int lineNumber) {

        currentFile = filename;
        setTitle("Source Viewer - " + filename);

        // grab cached file info if it's present
        // this can speed up full path calculation and provide replacement info
        SourceFileInfo curFileInfo = null;
        if (allFiles.containsKey(filename)) {
            curFileInfo = allFiles.get(filename);
        }
        sourceBorder.setFileInfo(curFileInfo);

        // find the source file
        String fullPath;
        if (curFileInfo != null) {
           fullPath = curFileInfo.path;
        } else {
           fullPath = getFullPath(filename);
        }

        // load the file
        if (fullPath != null) {
            try {
                BufferedReader reader = new BufferedReader(new FileReader(fullPath));
                StringBuffer code = new StringBuffer();
                String nextLine = reader.readLine();
                int lineCount = 1;
                int selStart = 0, selStop = 0;

                // read each line of the original source file
                while (nextLine != null) {

                    // insert replacement info if we have it
                    // (not needed w/ new graphical indicators
                    //appendReplacementInfo(code, lineCount, curFileInfo);

                    // append the current line
                    if (lineCount == lineNumber)
                        selStart = code.length();
                    code.append(nextLine);
                    if (lineCount == lineNumber)
                        selStop = code.length();
                    code.append(System.getProperty("line.separator"));
                    nextLine = reader.readLine();
                    lineCount++;
                }
                reader.close();

                sourceCode.setText(code.toString());
                sourceCode.select(selStart, selStop);
            } catch (Exception ex) {
                sourceCode.setText(ex.getMessage());
            }
        } else {
            sourceCode.setText("Cannot find file: " + filename);
            sourceBorder.setFileInfo(BLANK_FILE_INFO);
        }
        sourceCode.repaint();
    }

    private void appendReplacementInfo(StringBuffer code, int lineno, SourceFileInfo fileInfo) {
        Integer i = Integer.valueOf(lineno);
        if (fileInfo != null && (
                (fileInfo.singleCounts.containsKey(i) ||
                 fileInfo.doubleCounts.containsKey(i))
                )) {
            int k = 2;
            code.append("[");
            if (fileInfo.singleCounts.containsKey(i)) {
                for (int j=0; j<fileInfo.singleCounts.get(i).intValue(); j++, k++) {
                    code.append("s");
                }
            }
            if (fileInfo.doubleCounts.containsKey(i)) {
                for (int j=0; j<fileInfo.doubleCounts.get(i).intValue(); j++, k++) {
                    code.append("d");
                }
            }
            code.append("]");
            for (int j=0; j<(17-k); j++) {
                code.append(" ");
            }
        } else {
            for (int j=0; j<17; j++) {
                code.append(" ");
            }
        }
    }

    private void scanConfigInfo(ConfigTreeNode node) {
        // recursively scan for debug file information

        if (node.type == ConfigTreeNode.CNType.INSTRUCTION) {

            // it's an instruction; check for debug info (presence of filename)
            String fn = getFilename(node);
            if (!fn.equals("")) {

                // see if we've already added this file
                SourceFileInfo fi = null;
                if (allFiles.containsKey(fn)) {
                    fi = allFiles.get(fn);
                } else {
                    fi = new SourceFileInfo(fn);
                    fi.path = getFullPath(fn);
                    allFiles.put(fn, fi);
                }

                // extract line-specific replacement information
                int lineno = getLineNumber(node);
                if (node.status == ConfigTreeNode.CNStatus.SINGLE) {
                    fi.incrementSingle(lineno);
                }
                if (node.status == ConfigTreeNode.CNStatus.DOUBLE) {
                    fi.incrementDouble(lineno);
                }
            }
        } else {

            // it's not an instruction; recurse on its child nodes
            ConfigTreeNode child = null;
            Enumeration children = node.children();
            while (children.hasMoreElements()) {
                child = (ConfigTreeNode)children.nextElement();
                scanConfigInfo(child);
            }
        }
    }

    private void refreshFileList() {

        // add unique filenames to the file list
        int selectedFile = -1;
        DefaultListModel model = new DefaultListModel();
        for (String fn : allFiles.keySet()) {
            SourceFileInfo fi = allFiles.get(fn);
            if (fi.filename.equals(currentFile)) {
                selectedFile = model.size();
            }
            model.add(model.size(), fi);
        }
        sourceFiles.setModel(model);

        // make sure the currently-open file is selected
        if (selectedFile >= 0) {
            sourceFiles.setSelectedIndex(selectedFile);
        }
    }

    public static String getFilename(ConfigTreeNode node) {

        // extract the filename from the node's label
        String filename = "";
        Matcher m = REGEX_PATTERN.matcher(node.label);
        if (m.find()) {
            filename = m.group(1);
        }
        return filename;
    }

    public static String getFullPath(String filename) {
        
        // look through the search dirs stored in Util
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
        return fullPath;
    }

    public static int getLineNumber(ConfigTreeNode node) {

        // extract the line number from the node's label
        String linenoStr = "0";
        Matcher m = REGEX_PATTERN.matcher(node.label);
        if (m.find()) {
            linenoStr = m.group(2);
        }
        int lineno = 0;
        try {
            lineno = Integer.parseInt(linenoStr);
        } catch (NumberFormatException e) {};
        return lineno;
    }


    public void search(boolean findNext) {
        String text = searchBox.getText();

        // generally, start at the top ...
        int start = 0;

        // ... but if we're looking for the "next" match,
        // start after the current selection
        if (findNext && sourceCode.getSelectionEnd() > 0) {
            start = sourceCode.getSelectionEnd() + 1;
        }

        int pos = sourceCode.getText().indexOf(text, start);
        if (pos == -1) {
            // restart from the top
            pos = sourceCode.getText().indexOf(text);
        }
        if (pos >= 0) {
            // highlight found text
            sourceCode.setSelectionStart(pos);
            sourceCode.setSelectionEnd(pos+text.length());
        }
    }

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
        }
    }

    public void valueChanged(ListSelectionEvent e) {

        // switch to the newly-selected file
        Object obj = sourceFiles.getSelectedValue();
        if (obj == null) return;
        if (!(obj instanceof SourceFileInfo)) return;
        String fn = ((SourceFileInfo)obj).filename;
        loadFile(fn);
    }
}

