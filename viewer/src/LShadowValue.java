/**
 * LShadowValue
 *
 * holds detailed information about a shadow value
 */

import java.util.*;
import javax.swing.table.*;

public class LShadowValue extends AbstractTableModel {

    public String name;
    public String label;
    public String shadowValue;
    public String systemValue;
    public String absError;
    public String relError;
    public String address;
    public String size;
    public String shadowType;

    public double minAbsError;
    public double maxAbsError;
    public double sumAbsError;
    public double avgAbsError;
    public double minRelError;
    public double maxRelError;
    public double sumRelError;
    public double avgRelError;

    public int row, rows;
    public int col, cols;
    public List<LShadowValue> subElements;

    public LShadowValue() {
        name = ""; label = ""; shadowValue = ""; systemValue = "";
        absError = ""; relError = ""; address = ""; size = "";
        shadowType = "";
        minAbsError = Double.POSITIVE_INFINITY;
        maxAbsError = Double.NEGATIVE_INFINITY;
        sumAbsError = 0.0; avgAbsError = 0.0;
        minRelError = Double.POSITIVE_INFINITY;
        maxRelError = Double.NEGATIVE_INFINITY;
        sumRelError = 0.0; avgRelError = 0.0;
        row = 0; rows = 1; col = 0; cols = 1;
        subElements = new ArrayList<LShadowValue>();
    }

    public void addSubElement(LShadowValue entry) {
        try {
            if (entry.row >= rows) {
                rows = entry.row + 1;
            }
            if (entry.col >= cols) {
                cols = entry.col + 1;
            }
            subElements.add(entry);

            double abs = Double.parseDouble(entry.absError);
            double rel = Double.parseDouble(entry.relError);
            if (abs < minAbsError) { minAbsError = abs; }
            if (abs > maxAbsError) { maxAbsError = abs; }
            if (rel < minRelError) { minRelError = rel; }
            if (rel > maxRelError) { maxRelError = rel; }
            sumAbsError += abs;
            sumRelError += rel;
            avgAbsError = sumAbsError / getSubElementCount();
            avgRelError = sumRelError / getSubElementCount();
            absError = (Double.valueOf(sumAbsError)).toString();
            relError = (Double.valueOf(sumRelError)).toString();

            // TODO: fix address comparison (Java doesn't parse "0x"-prepended
            // hex addresses properly)
            //if (Integer.parseInt(entry.address,16) < Integer.parseInt(address,16)) {
                //address = entry.address;
            //}
        } catch (NumberFormatException ex) {
        }
    }

    public boolean isAggregate() {
        return subElements.size() > 0;
    }

    public int getSubElementCount() {
        return subElements.size();
    }

    public int getRowCount() {
        return rows;
    }

    public int getColumnCount() {
        return cols+1;
    }

    public String getColumnName(int column) {
        if (column == 0) {
            return "";
        } else {
            return ""+column;
        }
    }

    public Object getValueAt(int row, int column) {
        if (column == 0) {
            return ""+(row+1);
        } else {
            column = column - 1;
        }
        for (LShadowValue val : subElements) {
            if (val.row == row && val.col == column) {
                //System.out.println("(" + row + "," + column + ") : " + val.relError);
                //return val.relError;
                return MatrixModeListener.getValue(val);
            }
        }
        //System.out.println("(" + row + "," + column + ") : (none)");
        return "";
    }

    public LShadowValue getSubValueAt(int row, int column) {
        if (column == 0) {
            return null;
        } else {
            column = column - 1;
        }
        for (LShadowValue val : subElements) {
            if (val.row == row && val.col == column) {
                return val;
            }
        }
        return null;
    }

}

