/**
 * ShadowValueComparator
 *
 * handles comparison of shadow values (for sorting)
 */

import java.util.*;

public class ShadowValueComparator implements Comparator<LShadowValue> {

    private int column;
    private boolean ascending;

    public ShadowValueComparator(int column, boolean ascending) {
        this.column = column;
        this.ascending = ascending;
    }

    public int compare(LShadowValue v1, LShadowValue v2) {
        int order = 0;
        double val1, val2;
        try {
            switch (column) {
                case 0:     // name
                    if (v1.name.matches("\\w+\\(\\d+,\\d+\\)") &&
                        v2.name.matches("\\w+\\(\\d+,\\d+\\)")) {
                        // sort by matrix indices
                        String[] v1parts = v1.name.split("\\(|,|\\)");
                        String[] v2parts = v2.name.split("\\(|,|\\)");
                        int v1t, v2t;
                        if (v1parts[0].equals(v2parts[0])) {
                            v1t = Integer.parseInt(v1parts[1]);
                            v2t = Integer.parseInt(v2parts[1]);
                            if (v1t == v2t) {
                                v1t = Integer.parseInt(v1parts[2]);
                                v2t = Integer.parseInt(v2parts[2]);
                                if (v1t == v2t) {
                                    order = 0;
                                } else {
                                    order = v1t < v2t ? -1 : 1;
                                }
                            } else {
                                order = v1t < v2t ? -1 : 1;
                            }
                        } else {
                            order = v1parts[0].compareTo(v2parts[0]);
                        }
                    } else if (v1.name.matches("\\w+\\(\\d+\\)") &&
                               v2.name.matches("\\w+\\(\\d+\\)")) {
                        // sort by vector indices
                        String[] v1parts = v1.name.split("\\(|\\)");
                        String[] v2parts = v2.name.split("\\(|\\)");
                        int v1t, v2t;
                        if (v1parts[0].equals(v2parts[0])) {
                            v1t = Integer.parseInt(v1parts[1]);
                            v2t = Integer.parseInt(v2parts[1]);
                            if (v1t == v2t) {
                                order = 0;
                            } else {
                                order = v1t < v2t ? -1 : 1;
                            }
                        } else {
                            order = v1parts[0].compareTo(v2parts[0]);
                        }
                    } else {
                        // sort by variable name
                        order = v1.name.compareTo(v2.name);
                    }
                    break;
                case 1:     // shadow value (label)
                    val1 = Double.parseDouble(v1.label);
                    val2 = Double.parseDouble(v2.label);
                    order = (val1 == val2) ? v1.label.compareTo(v2.label) : (val1 < val2 ? -1: 1);
                    break;
                case 2:     // detailed shadow value
                    val1 = Double.parseDouble(v1.shadowValue);
                    val2 = Double.parseDouble(v2.shadowValue);
                    order = (val1 == val2) ? v1.shadowValue.compareTo(v2.shadowValue) : (val1 < val2 ? -1: 1);
                    break;
                /*
                 *case 2:     // system value
                 *    val1 = Double.parseDouble(v1.systemValue);
                 *    val2 = Double.parseDouble(v2.systemValue);
                 *    order = (val1 == val2) ? v1.systemValue.compareTo(v2.systemValue) : (val1 < val2 ? -1: 1);
                 *    break;
                 *case 3:     // absolute error
                 *    val1 = Double.parseDouble(v1.absError);
                 *    val2 = Double.parseDouble(v2.absError);
                 *    order = (val1 == val2) ? 0 : (val1 < val2 ? -1: 1);
                 *    break;
                 *case 4:     // relative error
                 *    val1 = Double.parseDouble(v1.relError);
                 *    val2 = Double.parseDouble(v2.relError);
                 *    order = (val1 == val2) ? 0 : (val1 < val2 ? -1: 1);
                 *    break;
                 */
                case 5:     // address
                    order = v1.address.compareTo(v2.address);
                    break;
                case 6:     // size
                    order = Integer.parseInt(v1.size) - Integer.parseInt(v2.size);
                    break;
                case 7:     // shadow value type
                    order = v1.shadowType.compareTo(v2.shadowType);
                    break;
                default:
                    order = 0;
                    break;
            }
        } catch (NumberFormatException e) {}
        if (!ascending)
            order = -order;
        return order;
    }

    public boolean equals(Object o) {
        return false;
    }

}

