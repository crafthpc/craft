/**
 * MessageComparator
 *
 * handles comparison of messages (for sorting)
 */

import java.util.*;

public class MessageComparator implements Comparator<LMessage> {
    
    private int column;
    private boolean ascending;

    public MessageComparator(int column, boolean ascending) {
        this.column = column;
        this.ascending = ascending;
    }

    public int compare(LMessage m1, LMessage m2) {
        int order = 0;
        try {
            switch (column) {
                case 0:     // time
                    order = Integer.parseInt(m1.time) - Integer.parseInt(m2.time);
                    break;
                case 1:     // priority
                    if (m1.priority.equals("ALL") && m2.priority.equals("ALL"))
                        order = 0;
                    else if (m1.priority.equals("ALL"))
                        order = 1;
                    else if (m2.priority.equals("ALL"))
                        order = -1;
                    else
                        order = Integer.parseInt(m1.priority) - Integer.parseInt(m2.priority);
                    break;
                case 2:     // type
                    order = m1.type.compareTo(m2.type);
                    break;
                case 3:     // address
                    order = m1.getAddress().compareTo(m2.getAddress());
                    break;
                case 4:     // function
                    order = m1.getFunction().compareTo(m2.getFunction());
                    break;
                case 5:     // label
                    order = m1.label.compareTo(m2.label);
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

