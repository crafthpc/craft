/**
 * ConfigTreeNode.java
 *
 * represents an entry in the configuration
 *
 */

import javax.swing.*;
import javax.swing.tree.*;

public class ConfigTreeNode extends DefaultMutableTreeNode {

    public enum CNType {
        NONE, APPLICATION, FUNCTION, BASIC_BLOCK, INSTRUCTION
    }

    public enum CNStatus {
        NONE, IGNORE, SINGLE, DOUBLE
    }

    public CNType type;
    public CNStatus status;
    public int number;
    public String label;

    public ConfigTreeNode parent;

    public ConfigTreeNode() {
        super();
        type = CNType.APPLICATION;
        status = CNStatus.NONE;
        number = 1;
        label = "Default App";
        parent = null;
    }

    public ConfigTreeNode(CNType t, CNStatus s, int num, String lbl) {
        super();
        type = t;
        status = s;
        number = num;
        label = lbl;
        parent = null;
    }

    public ConfigTreeNode(String configLine) {
        super();

        int pos = -1;

        if (configLine.contains("APPLICATION")) {
            type = CNType.APPLICATION;
        } else if (configLine.contains("FUNC")) {
            type = CNType.FUNCTION;
        } else if (configLine.contains("BBLK")) {
            type = CNType.BASIC_BLOCK;
        } else if (configLine.contains("INSN")) {
            type = CNType.INSTRUCTION;
        } else {
            type = CNType.NONE;
        }

        if (configLine.length() > 1) {
            switch (configLine.charAt(1)) {
                case ' ':   status = CNStatus.NONE;     break;
                case 'i':   status = CNStatus.IGNORE;   break;
                case 's':   status = CNStatus.SINGLE;   break;
                case 'd':   status = CNStatus.DOUBLE;   break;
                default:    status = CNStatus.NONE;     break;
            }
        }

        pos = configLine.indexOf('#');
        if (pos >= 0) {
            pos++;
            if (Character.isDigit(configLine.charAt(pos))) {
                int pos2 = pos;
                while ((pos2 < configLine.length()) && Character.isDigit(configLine.charAt(pos2))) {
                    pos2++;
                }
                if (pos2 > pos) {
                    number = Integer.parseInt(configLine.substring(pos, pos2));
                }
            }
        }

        if (configLine.length() > 2) {
            pos = configLine.indexOf(':');
            if (pos >= 0 && pos < configLine.length()-1) {
                label = configLine.substring(pos+1).trim();
            }
        } else {
            label = "(empty)";
        }

        parent = null;
    }

    public CNStatus getEffectiveStatus() {
        if (parent != null) {
            CNStatus parentStatus = parent.getEffectiveStatus();
            if (parentStatus != CNStatus.NONE) {
                return parentStatus;
            }
        }
        return status;
    }

    public void toggleNoneSingle() {
        switch (status) {
            case NONE:      status = CNStatus.SINGLE;       break;
            case IGNORE:    status = CNStatus.NONE;         break;
            case SINGLE:    status = CNStatus.NONE;         break;
            case DOUBLE:    status = CNStatus.NONE;         break;
            default:        status = CNStatus.NONE;         break;
        }
    }

    public void toggleIgnoreSingle() {
        switch (status) {
            case NONE:      status = CNStatus.IGNORE;       break;
            case IGNORE:    status = CNStatus.SINGLE;       break;
            case SINGLE:    status = CNStatus.IGNORE;       break;
            case DOUBLE:    status = CNStatus.IGNORE;       break;
            default:        status = CNStatus.IGNORE;       break;
        }
    }

    public void toggleAll() {
        switch (status) {
            case NONE:      status = CNStatus.IGNORE;       break;
            case IGNORE:    status = CNStatus.SINGLE;       break;
            case SINGLE:    status = CNStatus.DOUBLE;       break;
            case DOUBLE:    status = CNStatus.NONE;         break;
            default:        status = CNStatus.NONE;         break;
        }
    }

    public String formatFileEntry() {
        StringBuffer str = new StringBuffer();
        str.append('^');
        switch (status) {
            case NONE:      str.append(' ');    break;
            case IGNORE:    str.append('i');    break;
            case SINGLE:    str.append('s');    break;
            case DOUBLE:    str.append('d');    break;
            default:        str.append(' ');    break;
        }
        str.append(' ');
        switch (type) {
            case APPLICATION:       str.append("APPLICATION");    break;
            case FUNCTION:          str.append("  FUNC");         break;
            case BASIC_BLOCK:       str.append("    BBLK");       break;
            case INSTRUCTION:       str.append("      INSN");     break;
            default:                str.append("UNKNOWN");        break;
        }
        str.append(" #");
        str.append(number);
        str.append(": ");
        str.append(label);
        return str.toString();
    }

    public String toString() {
        StringBuffer str = new StringBuffer();
        switch (type) {
            case APPLICATION:       str.append("APPLICATION");    break;
            case FUNCTION:          str.append("  FUNC");         break;
            case BASIC_BLOCK:       str.append("    BBLK");       break;
            case INSTRUCTION:       str.append("      INSN");     break;
            default:                str.append("UNKNOWN");        break;
        }
        //str.append(" #");
        //str.append(number);
        str.append(": ");
        str.append(label);
        return str.toString();
    }

}

