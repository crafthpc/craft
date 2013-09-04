/**
 * RPrecConfigReport
 */

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.math.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.*;

public class RPrecConfigReport extends ConfigTreeIterator implements ConfigReport {

    private String insnText;
    private String execText;
    private long[] insnCountByPrecision;
    private long[] execCountByPrecision;
    private long[] insnCountBins;
    private long[] execCountBins;
    private String[] binLabels;

    private BarGraphPanel insnGraphPanel = null;
    private BarGraphPanel execGraphPanel = null;

    public void manipulate(ConfigTreeNode node) {
        if (node.type == ConfigTreeNode.CNType.INSTRUCTION) {
            long precision = node.getEffectivePrecision();
            if (node.status == ConfigTreeNode.CNStatus.RPREC ||
                 (precision >= 0 && precision <= 52)) {
                insnCountByPrecision[(int)precision] += node.insnCount;
                execCountByPrecision[(int)precision] += node.totalExecCount;
            }
        }
    }

    public void runReport(ConfigTreeNode node) {

        // ask for bin size
        int binSize = 1;
        String binSizeStr = JOptionPane.showInputDialog(null, "Enter bin size", "1");
        try {
            binSize = Integer.parseInt(binSizeStr);
        } catch (NumberFormatException ex) { }

        // ask whether to exclude the zero bin
        int resp = JOptionPane.showConfirmDialog(null, "Include zero-precision bin?", "Include zero-precision bin?",
                JOptionPane.YES_NO_OPTION);
        boolean excludeZero = false;
        if (resp == JOptionPane.NO_OPTION) {
            excludeZero = true;
        }

        // initialize count arrays
        insnCountByPrecision = new long[53];
        execCountByPrecision = new long[53];
        for (int i=0; i<53; i++) {
            insnCountByPrecision[i] = 0;
            execCountByPrecision[i] = 0;
        }

        // traverse config tree and populate count arrays
        node.preManipulate(this);

        // bin values
        int[] binMapping = new int[insnCountByPrecision.length];
        int numBins = (excludeZero ? 0 : 1) + (int)Math.ceil(52.0 / (float)binSize);
        binMapping[0] = (excludeZero ? -1 : 0);
        for (int i=1; i<insnCountByPrecision.length; i++) {
            binMapping[i] = (i-1)/binSize + (excludeZero ? 0 : 1);
        }

        // find min/max values for each bin and create labels
        int[] minVal = new int[numBins];
        int[] maxVal = new int[numBins];
        for (int i=0; i<numBins; i++) {
            minVal[i] = 99999;
            maxVal[i] = 0;
        }
        for (int i=0; i<insnCountByPrecision.length; i++) {
            int bin = binMapping[i];
            if (bin != -1) {
                if (i < minVal[bin]) minVal[bin] = i;
                if (i > maxVal[bin]) maxVal[bin] = i;
            }
        }
        binLabels = new String[numBins];
        for (int i=0; i<numBins; i++) {
            binLabels[i] = String.format("%d", minVal[i]);
            if (maxVal[i] > minVal[i]) {
                binLabels[i] = String.format("%d-%d", minVal[i], maxVal[i]);
            }
        }

        // prepare bar graph data
        insnCountBins = performBinning(insnCountByPrecision, binMapping, numBins);
        execCountBins = performBinning(execCountByPrecision, binMapping, numBins);
        
        // build the report text
        StringBuffer report = new StringBuffer();
        report.append("Instruction report (static):\n");
        for (int i=0; i<numBins; i++) {
            report.append(String.format("%5s: %d\n", binLabels[i], insnCountBins[i]));
        }
        insnText = report.toString();
        report = new StringBuffer();
        report.append("Execution report (dynamic):\n");
        for (int i=0; i<numBins; i++) {
            report.append(String.format("%5s: %d\n", binLabels[i], execCountBins[i]));
        }
        execText = report.toString();
    }

    public long[] performBinning(long[] original, int[] binMapping, int numBins) {
        assert(original.length == binMapping.length);
        long[] hist = new long[numBins];
        for (int i=0; i<numBins; i++) {
            hist[i] = 0;
        }
        for (int i=0; i<original.length; i++) {
            if (binMapping[i] != -1) {
                assert(binMapping[i] < binMapping.length);
                hist[binMapping[i]] += original[i];
            }
        }
        return hist;
    }

    public Component generateUI() {

        JTextArea insnBox = new JTextArea();
        insnBox.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        insnBox.setText(insnText);
        insnBox.setEditable(false);
        insnBox.setCaretPosition(0);

        JTextArea execBox = new JTextArea();
        execBox.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        execBox.setText(execText);
        execBox.setEditable(false);
        execBox.setCaretPosition(0);

        insnGraphPanel = new BarGraphPanel(insnCountBins, binLabels);
        execGraphPanel = new BarGraphPanel(execCountBins, binLabels);

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new GridLayout(2,2));
        mainPanel.add(insnGraphPanel);
        mainPanel.add(new JScrollPane(insnBox));
        mainPanel.add(execGraphPanel);
        mainPanel.add(new JScrollPane(execBox));
        return mainPanel;
    }

    public void saveGraphs() {
        if (insnGraphPanel == null) {
            insnGraphPanel = new BarGraphPanel(insnCountBins, binLabels);
        }
        insnGraphPanel.saveImageToFile(new File("./insn_rprec_graph.png"));
        if (execGraphPanel == null) {
            execGraphPanel = new BarGraphPanel(execCountBins, binLabels);
        }
        execGraphPanel.saveImageToFile(new File("./exec_rprec_graph.png"));
    }

    public String getTitle() {
        return "Reduced-precision Report";
    }

    public String getText() {
        return insnText + "\n" + execText;
    }
}

