/**
 * ViewerApp.java
 *
 * GUI viewer for fpanalysis data. Parses the XML log files produced by
 * fpinst and displays all messages in a filtered list view. Also extracts
 * information about individual instructions and aggregates various related
 * metrics, and displays shadow value results using appropriate views.
 *
 * Original author:
 *   Mike Lam (lam@cs.umd.edu)
 *   Professor Jeffrey K. Hollingsworth, UMD
 *   July 2009
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
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;
// }}}

public class ViewerApp extends JFrame implements ActionListener, DocumentListener {

    // {{{ member variables

    // shared interface elements
    private static final String DEFAULT_TITLE = "FPAnalysis Log Viewer";
    private static final Font DEFAULT_FONT_SANS_PLAIN = new Font("SansSerif", Font.PLAIN, 10);
    private static final Font DEFAULT_FONT_SANS_BOLD = new Font("SansSerif", Font.BOLD, 10);
    private static final Font DEFAULT_FONT_MONO_PLAIN = new Font("Monospaced", Font.PLAIN, 12);

    // main log file data structure
    public LLogFile mainLogFile;

    // main interface elements
    public JTabbedPane mainTabPanel;
    public JFileChooser fileChooser;
    public JPanel topPanel;
    public JTextArea sourceCode;
    public JScrollPane sourcePanel;
    public JTable matrixTable;
    public JScrollPane matrixPanel;
    public MatrixCellListener matrixListener;
    public TabListener mainTabListener;

    // message tab
    public JLabel messageLabel;
    public JTable messageList;
    public JScrollPane messageScroll;
    public JComboBox messageFilter;
    public JTextArea detailsLabel;
    public JList traceList;
    public MessageListener mListener;
    public TraceFrameListener tfListener;
    public TableHeaderListener mthListener;

    // instruction tab
    public JLabel instructionLabel;
    public JTable instructionList;
    public InstructionListener iListener;
    public TableHeaderListener ithListener;
    public TableRowSorter<InstructionModel> instructionSorter;
    public JPanel instructionFilter;
    public JTextField filterText;
    public JCheckBox filterInverse;

    // variable tab
    public JLabel variableLabel;
    public JTable variableList;
    public VariableListener vListener;
    public TableHeaderListener vthListener;

    // }}}
    
    // {{{ general initialization

    public ViewerApp() {
        super("");

        fileChooser = new JFileChooser();
        fileChooser.setCurrentDirectory(new File("."));

        Util.initSearchDirs();

        mainLogFile = null;

        buildMenuBar();
        buildMainInterface();
    }

    // }}}
    // {{{ initialize main interface

    public void buildMenuBar() {
        JMenu logMenu = new JMenu("File");
        logMenu.setMnemonic(KeyEvent.VK_F);
        logMenu.add(new OpenLogAction(this, "Open", null, Integer.valueOf(KeyEvent.VK_O)));
        logMenu.add(new MergeLogAction(this, "Merge", null, Integer.valueOf(KeyEvent.VK_M)));
        logMenu.add(new ResetAction(this, "Close All", null, Integer.valueOf(KeyEvent.VK_C)));

        JMenu reportMenu = new JMenu("Reports");
        reportMenu.setMnemonic(KeyEvent.VK_R);
        reportMenu.add(new ReportAction(this, new InstructionFuncReport(), 
                    "Instructions by Function Report (Instrumentation)", null, null));
        reportMenu.add(new ReportAction(this, new InstructionTypeReport(), 
                    "Instructions by Type Report (Instrumentation)", null, null));
        reportMenu.addSeparator();
        reportMenu.add(new ReportAction(this, new CancellationReport(),
                    "Cancellation Report (Runtime)", null, null));
        reportMenu.add(new ReportAction(this, new InstructionExecuteFuncReport(), 
                    "Instructions by Function Report (Runtime)", null, null));
        reportMenu.add(new ReportAction(this, new InstructionExecuteTypeReport(), 
                    "Instructions by Type Report (Runtime)", null, null));
        reportMenu.add(new ReportAction(this, new InstructionAssemblyReport(), 
                    "Instructions by Assembly Report (Runtime)", null, null));

        //reportMenu.addSeparator();
        //reportMenu.add(new RunProgramAction(this, "Run Program", null, Integer.valueOf(KeyEvent.VK_R)));
        
        JMenuBar menuBar = new JMenuBar();
        menuBar.add(logMenu);
        menuBar.add(reportMenu);
        setJMenuBar(menuBar);
    }

    public void buildMainInterface() {
        
        // source code viewer
        sourceCode = new JTextArea();
        sourceCode.setFont(DEFAULT_FONT_MONO_PLAIN);
        sourceCode.setBorder(new LineNumberedBorder(
                    LineNumberedBorder.LEFT_SIDE, LineNumberedBorder.LEFT_JUSTIFY));
        sourcePanel = new JScrollPane(sourceCode);
        sourcePanel.setVisible(false);

        // matrix viewer
        matrixTable = new JTable();
        matrixTable.setFont(DEFAULT_FONT_SANS_PLAIN);
        matrixTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
        matrixTable.setCellSelectionEnabled(true);
        matrixListener = new MatrixCellListener();
        matrixTable.addMouseListener(matrixListener);
        matrixPanel = new JScrollPane(matrixTable);
        matrixPanel.setVisible(false);

        // main structure
        topPanel = new JPanel();
        topPanel.setLayout(new BorderLayout());
        mainTabListener = new TabListener();
        mainTabListener.add(matrixTable);
        mainTabPanel = new JTabbedPane();
        mainTabPanel.addTab("Messages", buildMessagesTab());
        mainTabPanel.addTab("Instructions", buildInstructionsTab());
        mainTabPanel.addTab("Variables", buildVariablesTab());
        mainTabPanel.addChangeListener(mainTabListener);
        JSplitPane splitPanel = new JSplitPane(JSplitPane.VERTICAL_SPLIT,
                topPanel, mainTabPanel);

        getContentPane().add(splitPanel);
        setSize(1000,800);
        splitPanel.setDividerLocation(400);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);
        setTitle(DEFAULT_TITLE);

        splitPanel.revalidate();
    }

    // }}}
    // {{{ initialize message tab

    public JComponent buildMessagesTab() {

        // event/message list
        messageList = new JTable();
        messageList.setFont(DEFAULT_FONT_SANS_PLAIN);
        messageList.setShowHorizontalLines(false);
        messageList.setShowVerticalLines(false);
        messageList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        mthListener = new TableHeaderListener(messageList);
        messageList.getTableHeader().addMouseListener(mthListener);
        MessageHeaderRenderer mhRenderer = new MessageHeaderRenderer(mthListener);
        mhRenderer.setFont(DEFAULT_FONT_SANS_PLAIN);
        messageList.getTableHeader().setDefaultRenderer(mhRenderer);
        messageScroll = new JScrollPane(messageList);
        mainTabListener.add(messageList);
        messageLabel = new JLabel();
        messageLabel.setFont(DEFAULT_FONT_SANS_BOLD);
        JPanel filterPanel = new JPanel();
        filterPanel.setBackground(Color.WHITE);
        JLabel filterLabel = new JLabel("Filter by:");
        filterLabel.setFont(DEFAULT_FONT_SANS_BOLD);
        //messageFilter = new JComboBox(messageTypes);
        messageFilter = new JComboBox(new MessageFilterModel());
        messageFilter.setFont(DEFAULT_FONT_SANS_PLAIN);
        messageFilter.addActionListener(new MessageFilterListener(messageList));
        filterPanel.add(filterLabel);
        filterPanel.add(messageFilter);
        JPanel messageTopPanel = new JPanel();
        messageTopPanel.setBackground(Color.WHITE);
        messageTopPanel.setLayout(new BorderLayout());
        messageTopPanel.add(messageLabel, BorderLayout.CENTER);
        messageTopPanel.add(filterPanel, BorderLayout.EAST);
        JPanel messagePanel = new JPanel();
        messagePanel.setBackground(Color.WHITE);
        messagePanel.setLayout(new BorderLayout());
        messagePanel.add(messageTopPanel, BorderLayout.NORTH);
        messagePanel.add(messageScroll, BorderLayout.CENTER);

        // details/backtrace panel
        JPanel detailsPanel = new JPanel();
        detailsPanel.setBackground(Color.WHITE);
        detailsPanel.setLayout(new BorderLayout());
        detailsLabel = new JTextArea("");
        detailsLabel.setEditable(false);
        detailsLabel.setFont(DEFAULT_FONT_SANS_PLAIN);
        detailsPanel.add(detailsLabel, BorderLayout.NORTH);
        JLabel backtraceLabel =  new JLabel("Backtrace:");
        backtraceLabel.setFont(DEFAULT_FONT_SANS_BOLD);
        traceList = new JList();
        traceList.setFont(DEFAULT_FONT_SANS_PLAIN);
        JPanel tracePanel = new JPanel();
        tracePanel.setBackground(Color.WHITE);
        tracePanel.setLayout(new BorderLayout());
        tracePanel.add(backtraceLabel, BorderLayout.NORTH);
        tracePanel.add(traceList, BorderLayout.CENTER);
        detailsPanel.add(tracePanel, BorderLayout.CENTER);
        JScrollPane detailsScroll = new JScrollPane(detailsPanel);
        JSplitPane controlPanel = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
                messagePanel, detailsScroll);
        controlPanel.setDividerLocation(550);

        // event listeners
        mListener = new MessageListener(topPanel, messageList, detailsLabel, traceList);
        messageList.getSelectionModel().addListSelectionListener(mListener);
        tfListener = new TraceFrameListener(traceList, this /*, sourceCode*/);
        traceList.getSelectionModel().addListSelectionListener(tfListener);

        return controlPanel;
    }

    // }}}
    // {{{ initialize instruction tab

    public JComponent buildInstructionsTab() {

        // event/message list
        instructionList = new JTable();
        instructionList.setFont(DEFAULT_FONT_SANS_PLAIN);
        instructionList.setShowHorizontalLines(false);
        instructionList.setShowVerticalLines(false);
        instructionList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        ithListener = new TableHeaderListener(instructionList);
        //instructionList.getTableHeader().addMouseListener(ithListener);
        InstructionHeaderRenderer ihRenderer = new InstructionHeaderRenderer(ithListener);
        ihRenderer.setFont(DEFAULT_FONT_SANS_PLAIN);
        //instructionList.getTableHeader().setDefaultRenderer(ihRenderer);
        mainTabListener.add(instructionList);
        JScrollPane instructionScroll = new JScrollPane(instructionList);
        instructionLabel = new JLabel();
        instructionLabel.setFont(DEFAULT_FONT_SANS_BOLD);
        filterText = new JTextField(30);
        filterText.getDocument().addDocumentListener(this);
        filterInverse = new JCheckBox("Inverse");
        filterInverse.addActionListener(this);
        instructionFilter = new JPanel();
        instructionFilter.add(new JLabel("Filter (regex): "));
        instructionFilter.add(filterText);
        instructionFilter.add(filterInverse);
        JPanel instructionPanel = new JPanel();
        instructionPanel.setBackground(Color.WHITE);
        instructionPanel.setLayout(new BorderLayout());
        instructionPanel.add(instructionLabel, BorderLayout.NORTH);
        instructionPanel.add(instructionScroll, BorderLayout.CENTER);
        instructionPanel.add(instructionFilter, BorderLayout.SOUTH);
        instructionPanel.setBorder(BorderFactory.createLoweredBevelBorder());

        // event listeners
        iListener = new InstructionListener(instructionList, this /*, detailsLabel*/);
        instructionList.getSelectionModel().addListSelectionListener(iListener);
        instructionList.addMouseListener(iListener);

        return instructionPanel;
    }

    // }}}
    // {{{ initialize variable tab

    public JComponent buildVariablesTab() {

        // matrix mode selection
        JComboBox matrixModeBox = new JComboBox(MatrixModeListener.getValidOptions());
        matrixModeBox.addActionListener(new MatrixModeListener(matrixTable));
        matrixModeBox.setSelectedItem(MatrixModeListener.getMainMode());
        JPanel matrixModePanel = new JPanel();
        matrixModePanel.setBackground(Color.WHITE);
        matrixModePanel.add(new JLabel("Show in matrix view:"));
        matrixModePanel.add(matrixModeBox);

        // event/message list
        variableList = new JTable();
        variableList.setFont(DEFAULT_FONT_SANS_PLAIN);
        variableList.setShowHorizontalLines(false);
        variableList.setShowVerticalLines(false);
        variableList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        vthListener = new TableHeaderListener(variableList);
        variableList.getTableHeader().addMouseListener(vthListener);
        VariableHeaderRenderer ihRenderer = new VariableHeaderRenderer(vthListener);
        ihRenderer.setFont(DEFAULT_FONT_SANS_PLAIN);
        variableList.getTableHeader().setDefaultRenderer(ihRenderer);
        mainTabListener.add(variableList);
        JScrollPane variableScroll = new JScrollPane(variableList);
        variableLabel = new JLabel();
        variableLabel.setFont(DEFAULT_FONT_SANS_BOLD);
        JPanel variablePanel = new JPanel();
        variablePanel.setBackground(Color.WHITE);
        variablePanel.setLayout(new BorderLayout());
        variablePanel.add(variableLabel, BorderLayout.NORTH);
        variablePanel.add(variableScroll, BorderLayout.CENTER);
        variablePanel.add(matrixModePanel, BorderLayout.SOUTH);
        variablePanel.setBorder(BorderFactory.createLoweredBevelBorder());

        // event listeners
        vListener = new VariableListener(variableList, this /*, detailsLabel*/);
        variableList.getSelectionModel().addListSelectionListener(vListener);
        variableList.addMouseListener(vListener);

        return variablePanel;
    }

    // }}}

    // {{{ helper/utility functions

    public void openLogFile(File file) {
        mainLogFile = null;
        mergeLogFile(file);
    }

    public void reset() {
        mainLogFile = null;
        messageList.setModel(new DefaultTableModel());
        ((MessageFilterModel)messageFilter.getModel()).setLogFile(null);
        instructionList.setModel(new DefaultTableModel());
        variableList.setModel(new DefaultTableModel());
        messageList.revalidate();
        messageFilter.revalidate();
        instructionList.revalidate();
        variableList.revalidate();
        messageLabel.setText("0 total message(s):");
        instructionLabel.setText("0 instruction(s):");
        variableLabel.setText("0 variable(s):");
    }

    public void clearTopPanel() {
        topPanel.removeAll();
        sourcePanel.setVisible(false);
        matrixPanel.setVisible(false);
        topPanel.revalidate();
        topPanel.repaint();
    }

    public void applyFilter() {
        RowFilter<InstructionModel, Object> rf = null;
        try {
            rf = RowFilter.regexFilter(filterText.getText(), 4);
        } catch (PatternSyntaxException e) {
            return;
        }
        if (filterInverse.isSelected()) {
            instructionSorter.setRowFilter(RowFilter.notFilter(rf));
        } else {
            instructionSorter.setRowFilter(rf);
        }
    }

    // }}}

    // {{{ event listeners

    public void changedUpdate(DocumentEvent e) {
        applyFilter();
    }

    public void insertUpdate(DocumentEvent e) {
        applyFilter();
    }

    public void removeUpdate(DocumentEvent e) {
        applyFilter();
    }

    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == filterInverse) {
            applyFilter();
        }
    }

    // }}}

    // {{{ mergeConfigFile
    public void mergeConfigFile(File file) {
        if (!file.exists()) {
            JOptionPane.showMessageDialog(null, "I/O error: " + file.getAbsolutePath() + " does not exist!");
            return;
        }
        ConfigEditorApp capp = new ConfigEditorApp();
        ConfigTreeNode cnode = capp.openConfigFile(file);
        for (ConfigTreeNode node : capp.mainConfigEntries) {
            if (node.type == ConfigTreeNode.CNType.INSTRUCTION) {
                String addr = Util.extractRegex(node.label, "0x[0-9a-fA-F]+", 0);
                if (addr != null) {
                    LInstruction insn = mainLogFile.instructionsByAddress.get(addr);
                    if (insn != null) {
                        insn.rstatus = ConfigTreeNode.status2Str(node.getEffectiveStatus());
                    }
                }
            }
        }
        capp.dispose();
    } // }}}
    // {{{ mergeLogFile
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

            if (mainLogFile == null) {

                // this is the first (main) log file
                mainLogFile = logfile;

                // new message/instruction/variable models
                MessageModel mm = new MessageModel(logfile, messageLabel);
                messageList.setModel(mm);
                mm.setPreferredColumnSizes(messageList.getColumnModel());
                messageFilter.setSelectedIndex(5);
                ((MessageFilterModel)messageFilter.getModel()).setLogFile(logfile);
                InstructionModel im = new InstructionModel(logfile);
                instructionList.setModel(im);
                im.setPreferredColumnSizes(instructionList.getColumnModel());
                instructionSorter = new TableRowSorter<InstructionModel>(im);
                instructionList.setRowSorter(instructionSorter);
                ShadowValueModel vm = new ShadowValueModel(logfile);
                variableList.setModel(vm);
                vm.setPreferredColumnSizes(variableList.getColumnModel());

                // shared stuff
                setTitle(DEFAULT_TITLE + (logfile.appname.equals("") ? "" : " - " + logfile.appname));
                if (file.getParentFile() != null) {
                    fileChooser.setCurrentDirectory(file.getParentFile());
                }

                // choose tab to be focused (don't do this on merges, just the
                // main file)
                int num_msg = ((MessageModel)messageList.getModel()).getSize();
                int num_instr = ((InstructionModel)instructionList.getModel()).getSize();
                int num_var = ((ShadowValueModel)variableList.getModel()).getSize();
                if (num_var > num_msg /*&& num_var > num_instr*/) {
                    mainTabPanel.setSelectedIndex(2);
                //} else if (num_instr > num_msg [>&& num_instr > num_var<]) {
                } else {
                    mainTabPanel.setSelectedIndex(1);
                    // make matrix panel visible
                }

            } else {

                // this is a merged logfile, so call the merge routine then
                // refresh the models
                if (mainLogFile.appname.equals("") || logfile.appname.equals("") ||
                        mainLogFile.appname.equals(logfile.appname)) {
                    mainLogFile.merge(logfile);
                    ((MessageModel)messageList.getModel()).refreshData();
                    ((MessageFilterModel)messageFilter.getModel()).refreshData();
                    ((InstructionModel)instructionList.getModel()).refreshData();
                    ((ShadowValueModel)variableList.getModel()).refreshData();
                } else {
                    throw new Exception("Incompatible merge files; to force, change \"appname\" in one of the files");
                }
            }

            // refresh counts
            //messageLabel.setText(((MessageModel)messageList.getModel()).getSize() + " total message(s):");
            messageLabel.setText(mainLogFile.messages.size() + " total message(s):");
            instructionLabel.setText(((InstructionModel)instructionList.getModel()).getSize() + " instruction(s):");
            variableLabel.setText(((ShadowValueModel)variableList.getModel()).getSize() + " variable(s):");

            // repaint interface
            messageList.revalidate();
            messageFilter.revalidate();
            instructionList.revalidate();
            variableList.revalidate();
            Util.addSearchDir(file.getParentFile());
            clearTopPanel();

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
    } // }}}
    // {{{ openSourceFile
    public void openSourceFile(String filename, String linenoStr) {

        topPanel.removeAll();

        if (filename.equals("")) {
            sourcePanel.setVisible(false);
        } else {
            // make the source code window visible
            topPanel.add(sourcePanel, BorderLayout.CENTER);
            sourcePanel.setVisible(true);
            matrixPanel.setVisible(false);
            topPanel.revalidate();

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
            int lineno = 0;
            try {
                lineno = Integer.parseInt(linenoStr);
            } catch (NumberFormatException e) {};
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
                    reader.close();
                    sourceCode.setText(code.toString());
                    sourceCode.select(selStart, selStop);
                } catch (Exception ex) {
                    sourceCode.setText(ex.getMessage());
                }
            } else {
                sourceCode.setText("Cannot find file: " + filename);
            }
        }

        topPanel.revalidate();
    } // }}}
    // {{{ viewShadowValue
    public void viewShadowValue(LShadowValue value) {

        topPanel.removeAll();
        sourcePanel.setVisible(false);
        if (value.rows == 1 && value.cols == 1) {
            // not an array or matrix
            matrixPanel.setVisible(false);
        } else {
            // make matrix panel visible
            topPanel.add(matrixPanel, BorderLayout.CENTER);
            matrixPanel.setVisible(true);
            sourcePanel.setVisible(false);
            topPanel.revalidate();

            matrixTable.setModel(value);
            matrixListener.setModel(value);

            ColumnResizer.resizeColumns(matrixTable);
        }

        topPanel.revalidate();
        topPanel.repaint();
    } // }}}
    // {{{ filterByInstruction
    public void filterByInstruction(LInstruction inst) {
        mainTabPanel.setSelectedIndex(0);
        messageFilter.setSelectedItem("(address)");
        MessageModel model = (MessageModel)messageList.getModel();
        model.typeFilter = "(address)";
        model.addressFilter = inst.address;
        model.refreshData();
        messageList.repaint();
        topPanel.revalidate();
        openSourceFile(inst.file, inst.lineno);
    } // }}}

    // {{{ main method
    public static void main(String[] args) {
        ViewerApp app = new ViewerApp();
        java.util.List<File> files = new ArrayList<File>();
        int i;
        String tag;
        File instFile;
        app.reset();
        boolean cancelOnly = false;
        boolean instTypeOnly = false;
        boolean summaryOnly = false;

        // parse command-line args
        for (i = 0; i < args.length; i++) {
            if (args[i].equals("-c")) {
                cancelOnly = true;
            } else if (args[i].equals("-i")) {
                instTypeOnly = true;
            } else if (args[i].equals("-s")) {
                summaryOnly = true;
            } else {
                File f = new File(args[i]);
                if (f.exists()) {
                    files.add(f);
                } else {
                    JOptionPane.showMessageDialog(null, "File not found: " + f.getName());
                }
            }
        }

        if (cancelOnly) {
            // print cancellation report for given cancellation logs
            for (File f : files) {
                app.openLogFile(f);
                tag = f.getAbsolutePath().replaceAll("_dcancel\\.log", "");
                instFile = new File(tag + "_cinst.log");
                if (instFile.exists()) {
                    app.mergeLogFile(instFile);
                }
                tag = f.getName().replaceAll("_dcancel\\.log", "");
                Report report = new CancellationReport();
                report.runReport(app.mainLogFile);
                System.out.println(report.getText());
            }
        } else if (instTypeOnly) {
            for (File f : files) {
                app.openLogFile(f);
                Report report = new InstructionTypeReport();
                report.runReport(app.mainLogFile);
                System.out.println("INSTRUCTION TYPE REPORT:\n");
                System.out.println(report.getText());
                report = new InstructionExecuteTypeReport();
                report.runReport(app.mainLogFile);
                System.out.println("\nEXECUTION TYPE REPORT:\n");
                System.out.println(report.getText());
                report = new InstructionAssemblyReport();
                report.runReport(app.mainLogFile);
                System.out.println("\nEXECUTION ASSEMBLY REPORT:\n");
                System.out.println(report.getText());
            }
        } else if (summaryOnly) {
            for (File f : files) {
                app.openLogFile(f);
                Report report = new SummaryReport();
                report.runReport(app.mainLogFile);
                System.out.println("SUMMARY REPORT:\n");
                System.out.println(report.getText());
            }
        } else {
            // open all given log files
            for (File f : files) {
                if (Util.getExtension(f).equals("cfg")) {
                    app.mergeConfigFile(f);
                } else if (Util.getExtension(f).equals("log")) {
                    app.mergeLogFile(f);
                } else {
                    JOptionPane.showMessageDialog(null, "Invalid file extension: " + f.getName());
                }
            }
            app.setVisible(true);
        }
    } // }}}

}

