/**
 * InstructionComparator
 *
 * handles comparison of instructions (for sorting)
 */

import java.util.*;

public class InstructionComparator implements Comparator<LInstruction> {

    private int column;
    private boolean ascending;

    public InstructionComparator(int column, boolean ascending) {
        this.column = column;
        this.ascending = ascending;
    }

    public int compare(LInstruction i1, LInstruction i2) {
        int order = 0;
        try {
            switch (column) {
                case 0:     // id
                    order = Integer.parseInt(i1.id) - Integer.parseInt(i2.id);
                    break;
                case 1:     // address
                    order = i1.address.compareTo(i2.address);
                    // TODO: fix this
                    //order = Integer.parseInt(i1.address, 16) - Integer.parseInt(i2.address, 16);
                    break;
                case 2:     // function
                    order = i1.function.compareTo(i2.function);
                    break;
                case 3:     // source
                    if (i1.file.equals(i2.file)) {
                        order = Integer.parseInt(i1.lineno) - Integer.parseInt(i2.lineno);
                    } else {
                        order = i1.file.compareTo(i2.file);
                    }
                    break;
                case 4:     // disassembly
                    order = i1.disassembly.compareTo(i2.disassembly);
                    break;
                case 5:     // replacement status
                    order = i1.rstatus.compareTo(i2.rstatus);
                    break;
                case 6:     // count
                    order = i1.count - i2.count;
                    break;
                case 7:     // cancellations
                    if (i1.totalCancels > 0 && i2.totalCancels > 0)
                        order = i1.totalCancels - i2.totalCancels;
                    else
                        order = i1.cancellations - i2.cancellations;
                    break;
                case 8:     // samples
                    order = i1.cancellations - i2.cancellations;
                    break;
                case 9:     // ratio
                    order = (i1.ratio == i2.ratio) ? 0 : (i1.ratio < i2.ratio ? -1: 1);
                    break;
                case 10:     // average digits
                    order = (i1.averageDigits == i2.averageDigits) ? 0 : (i1.averageDigits < i2.averageDigits ? -1: 1);
                    break;
                case 11:    // min
                    order = (i1.min == i2.min) ? 0 : (i1.min < i2.min ? -1: 1);
                    break;
                case 12:    // max
                    order = (i1.max == i2.max) ? 0 : (i1.max < i2.max ? -1: 1);
                    break;
                case 13:    // range
                    order = (i1.range == i2.range) ? 0 : (i1.range < i2.range ? -1: 1);
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

