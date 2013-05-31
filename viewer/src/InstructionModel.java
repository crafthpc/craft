/**
 * InstructionModel
 *
 * handles table data for "Instructions" tab
 */

import java.text.*;
import java.util.*;
import javax.swing.event.*;
import javax.swing.table.*;

public class InstructionModel extends AbstractTableModel {

    public LLogFile logfile;
    public java.util.List<LInstruction> instructions;
    public NumberFormat ratioFormatter;
    public NumberFormat avgDigitsFormatter;

    public InstructionModel(LLogFile log) {
        logfile = log;
        ratioFormatter = new DecimalFormat("0.####");
        avgDigitsFormatter = new DecimalFormat("0.##");
        refreshData();
    }

    public void refreshData() {
        instructions = new ArrayList<LInstruction>();
        for (LInstruction i : logfile.instructions.values()) {
            instructions.add(i);
            i.count = 0;
            i.cancellations = 0;
        }
        for (LMessage msg : logfile.messages) {
            if (msg.instruction != null) {
                if (msg.backtrace != null && msg.backtrace.getSize() > 0) {
                    LFrame f = msg.backtrace.frames.get(0);
                    if (msg.instruction.function.equals(""))
                        msg.instruction.function = f.function;
                    if (msg.instruction.file.equals(""))
                        msg.instruction.file = f.file;
                    if (msg.instruction.lineno.equals(""))
                        msg.instruction.lineno = f.lineno;
                }
                if (msg.type.equals("InstCount")) {
                    if (msg.instruction.disassembly.equals(""))
                        msg.instruction.disassembly = msg.label;
                    msg.instruction.count = Integer.parseInt(msg.priority);
                    msg.instruction.updateRatio();
                } else if (msg.type.equals("Cancellation")) {
                    msg.instruction.cancellations += 1;
                    msg.instruction.updateRatio();
                } else if (msg.type.equals("Summary") && msg.label.equals("CANCEL_DATA")) {
                    String[] data = msg.details.split("\n|=");
                    if (data.length > 2) {
                        msg.instruction.totalCancels = Integer.parseInt(data[2]);
                        msg.instruction.updateRatio();
                    }
                    if (data.length > 6) {
                        msg.instruction.averageDigits = Util.parseDouble(data[6]);
                    }
                } else if (msg.type.equals("Summary") && msg.label.equals("RANGE_DATA")) {
                    String[] data = msg.details.split("\n|=");
                    if (data.length > 2) {
                        msg.instruction.min = Util.parseDouble(data[2]);
                    }
                    if (data.length > 4) {
                        msg.instruction.max = Util.parseDouble(data[4]);
                        msg.instruction.range = msg.instruction.max - msg.instruction.min;
                    }
                    if (data.length > 6) {
                        msg.instruction.range = Util.parseDouble(data[6]);
                    }
                }
            }
        }
        Collections.sort(instructions, new InstructionComparator(0, true));
    }

    public Object getValueAt(int row, int col) {
        LInstruction instr = instructions.get(row);
        switch (col) {
            case 0: return instr.id;
            case 1: return instr.address;
            case 2: return instr.function;
            case 3: return instr.getSource();
            case 4: return instr.disassembly;
            case 5: return instr.rstatus;
            //case 6: return (Integer.valueOf(instr.count)).toString();
            case 6: return Integer.valueOf(instr.count);
            case 7: if (instr.totalCancels > 0)
                        //return (Integer.valueOf(instr.totalCancels)).toString();
                        return Integer.valueOf(instr.totalCancels);
                    else
                        //return (Integer.valueOf(instr.cancellations)).toString();
                        return Integer.valueOf(instr.cancellations);
            //case 8: return (Integer.valueOf(instr.cancellations)).toString();
            case 8: return Integer.valueOf(instr.cancellations);
            //case 9: return ratioFormatter.format(instr.ratio);
            //case 10: return avgDigitsFormatter.format(instr.averageDigits);
            case 9: return Double.valueOf(instr.ratio);
            case 10: return Double.valueOf(instr.averageDigits);
            //case 11: return (Double.valueOf(instr.min)).toString();
            //case 12: return (Double.valueOf(instr.max)).toString();
            //case 13: return (Double.valueOf(instr.range)).toString();
            case 11: return Double.valueOf(instr.min);
            case 12: return Double.valueOf(instr.max);
            case 13: return Double.valueOf(instr.range);
            default: return "";
        }
    }

    public int getSize() {
        return instructions.size();
    }

    public int getColumnCount() {
        return 14;
    }

    public int getRowCount() {
        return instructions.size();
    }

    public String getColumnName(int col) {
        switch (col) {
            case 0: return "ID";
            case 1: return "Address";
            case 2: return "Function";
            case 3: return "Source";
            case 4: return "Assembly";
            case 5: return "Status";
            case 6: return "Count";
            case 7: return "Cancels";
            case 8: return "Samples";
            case 9: return "Ratio";
            case 10: return "AvgDigits";
            case 11: return "Min";
            case 12: return "Max";
            case 13: return "Range";
            default: return "";
        }
    }

    public Class getColumnClass(int col) {
        return getValueAt(0,col).getClass();
    }

    public void setPreferredColumnSizes(TableColumnModel model) {
        model.getColumn(0).setPreferredWidth(20);
        model.getColumn(1).setPreferredWidth(75);
        model.getColumn(2).setPreferredWidth(75);
        model.getColumn(3).setPreferredWidth(100);
        model.getColumn(4).setPreferredWidth(200);
        model.getColumn(5).setPreferredWidth(75);
        model.getColumn(6).setPreferredWidth(40);
        model.getColumn(7).setPreferredWidth(40);
        model.getColumn(8).setPreferredWidth(40);
        model.getColumn(9).setPreferredWidth(40);
        model.getColumn(10).setPreferredWidth(40);
        model.getColumn(11).setPreferredWidth(60);
        model.getColumn(12).setPreferredWidth(60);
    }

    public boolean isCellEditable(int row, int col) {
        return false;
    }

    public void setValueAt(Object value, int row, int col) {
        return;
    }

    public void addTableModelListener(TableModelListener l) { }
    public void removeTableModelListener(TableModelListener l) { }

    public void sort(int column, boolean ascending) {
        Collections.sort(instructions, new InstructionComparator(column, ascending));
    }

}

