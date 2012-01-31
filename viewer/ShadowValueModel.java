/**
 * ShadowValueModel
 *
 * handles table data for "Variable" tab
 */

import java.util.*;
import java.util.regex.*;
import javax.swing.event.*;
import javax.swing.table.*;

public class ShadowValueModel extends AbstractTableModel {

    public LLogFile logfile;
    public java.util.List<LShadowValue> values;

    public ShadowValueModel(LLogFile log) {
        logfile = log;
        refreshData();
    }

    public String captureGroup(String text, String regex, int groupNum) {
        Pattern pattern = Pattern.compile(regex);
        Matcher matcher = pattern.matcher(text);
        String result = null;
        if (matcher.find()) {
            result = matcher.group(groupNum);
        }
        return result;
    }

    public void refreshData() {
        boolean found, isSubElement;
        String rootName;
        values = new ArrayList<LShadowValue>();
        for (LMessage msg : logfile.messages) {
            if (msg.type.equals("ShadowVal")) {
                isSubElement = false;
                try {
                    // build shadow value element
                    LShadowValue val = new LShadowValue();

                    val.name = msg.label;
                    val.label = captureGroup(msg.details, "label=(.*)", 1);
                    val.shadowValue = captureGroup(msg.details, "value=(.*)", 1);
                    val.systemValue = captureGroup(msg.details, "system value=(.*)", 1);
                    val.absError = captureGroup(msg.details, "abs error=(.*)", 1);
                    val.relError = captureGroup(msg.details, "rel error=(.*)", 1);
                    val.address = captureGroup(msg.details, "address=(.*)", 1);
                    val.size = captureGroup(msg.details, "size=(.*)", 1);
                    val.shadowType = captureGroup(msg.details, "type=(.*)", 1);

                    // vector/matrix element
                    if (val.name.matches("\\w+\\(\\d+\\)") ||
                        val.name.matches("\\w+\\(\\d+,\\d+\\)")) {
                        String[] vparts = val.name.split("\\(|,|\\)");
                        rootName = vparts[0];
                        isSubElement = true;
                        val.row = Integer.parseInt(vparts[1]);
                        if (vparts.length == 3) {
                            val.col = Integer.parseInt(vparts[2]);
                        }

                        // look for main entry and update it (or create it)
                        if (isSubElement) {
                            found = false;
                            for (LShadowValue msg2 : values) {
                                if (msg2.name.equals(rootName) &&
                                    msg2.shadowType.equals(val.shadowType)) {
                                    msg2.addSubElement(val);
                                    found = true;
                                }
                            }
                            if (!found) {
                                LShadowValue root = new LShadowValue();
                                root.name = rootName;
                                root.shadowValue = "(aggregate)";
                                root.systemValue = "(aggregate)";
                                root.absError = "0";
                                root.relError = "0";
                                root.address = val.address;
                                root.size = val.size;
                                root.shadowType = val.shadowType;
                                root.addSubElement(val);
                                values.add(root);
                            }
                        }
                    }

                    // check for duplicates (this could be expensive...)
                    found = false;
                    for (LShadowValue msg2 : values) {
                        if (msg2.name.equals(val.name) &&
                            msg2.shadowType.equals(val.shadowType)) {
                            found = true;
                            break;
                        }
                    }

                    // add to collection
                    if (!found && !isSubElement) {
                        values.add(val);
                    }
                } catch (Exception ex) {
                    System.out.println(ex.getMessage() + "\n" + ex.getStackTrace().toString());
                }
            }
        }
        Collections.sort(values, new ShadowValueComparator(0, true));
    }

    public Object getValueAt(int row, int col) {
        LShadowValue val = values.get(row);
        if (val == null) {
            return "";
        }
        switch (col) {
            case 0: return val.name;
            //case 1: return (val.label.length() > val.shadowValue.length()+10 ? val.shadowValue : val.label);
            case 1: return val.label;
            case 2: return val.shadowValue;
            //case 2: return val.systemValue;
            //case 3: return val.absError;
            //case 4: return val.relError;
            case 3: return val.address;
            case 4: return val.size;
            case 5: return val.shadowType;
            default: return "";
        }
    }

    public int getSize() {
        return values.size();
    }

    public int getColumnCount() {
        return 6;
    }

    public int getRowCount() {
        return values.size();
    }

    public String getColumnName(int col) {
        switch (col) {
            case 0: return "Name";
            case 1: return "Shadow Value";
            case 2: return "Detailed Shadow Value";
            //case 2: return "System Value";
            //case 3: return "Absolute Error";
            //case 4: return "Relative Error";
            case 3: return "Address";
            case 4: return "Size (bytes)";
            case 5: return "Shadow Type";
            default: return "";
        }
    }

    public Class getColumnClass(int col) {
        return getValueAt(0,col).getClass();
    }

    public void setPreferredColumnSizes(TableColumnModel model) {
        model.getColumn(0).setPreferredWidth(50);
        model.getColumn(1).setPreferredWidth(150);
        model.getColumn(2).setPreferredWidth(250);
        //model.getColumn(2).setPreferredWidth(150);
        //model.getColumn(3).setPreferredWidth(80);
        //model.getColumn(4).setPreferredWidth(80);
        model.getColumn(3).setPreferredWidth(50);
        model.getColumn(4).setPreferredWidth(50);
        model.getColumn(5).setPreferredWidth(100);
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
        Collections.sort(values, new ShadowValueComparator(column, ascending));
    }

}

