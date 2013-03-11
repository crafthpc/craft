/**
 * ConfigTreeNode.java
 *
 * represents an entry in the configuration
 *
 */

import java.util.*;
import javax.swing.*;
import javax.swing.tree.*;

public class ConfigTreeNode extends DefaultMutableTreeNode {

    public enum CNType {
        NONE, APPLICATION, MODULE, FUNCTION, BASIC_BLOCK, INSTRUCTION
    }

    public enum CNStatus {
        NONE, IGNORE, SINGLE, DOUBLE, CANDIDATE
    }

    public static String Type2Str(CNType type) {
        String str = "UNKNOWN";
        switch (type) {
            case APPLICATION:       str = "APPL";    break;
            case MODULE:            str = "MODL";    break;
            case FUNCTION:          str = "FUNC";    break;
            case BASIC_BLOCK:       str = "BBLK";    break;
            case INSTRUCTION:       str = "INSN";    break;
        }
        return str;
    }

    public static String Status2Str(CNStatus status) {
        String str = "UNKNOWN";
        switch (status) {
            case NONE:      str = "NONE";    break;
            case IGNORE:    str = "IGNR";    break;
            case SINGLE:    str = "SING";    break;
            case DOUBLE:    str = "DOUB";    break;
            case CANDIDATE: str = "CAND";    break;
        }
        return str;
    }

    public CNType type;
    public CNStatus status;
    public boolean candidate;
    public int number;
    public long insnCount;
    public long insnExecCount;  // for code coverage
    public long totalExecCount;
    public Map<CNStatus, Long> execCount;
    public String label;
    public String address;
    public String regexTag;     // type, id, and address
    public double error;        // from searches
    public boolean tested;      // from searches

    public ConfigTreeNode() {
        super();
        type = CNType.APPLICATION;
        status = CNStatus.NONE;
        candidate = false;
        number = 1;
        insnCount = -1;
        resetExecCounts();
        label = "Default App";
        address = "";
        regexTag = "";
        error = 0.0;
        tested = false;
    }

    public ConfigTreeNode(CNType t, CNStatus s, int num, String lbl) {
        super();
        type = t;
        status = s;
        candidate = false;
        number = num;
        insnCount = -1;
        resetExecCounts();
        label = lbl;
        address = "";
        regexTag = "";
        error = 0.0;
        tested = false;
    }

    public ConfigTreeNode(String configLine) {
        super();

        int pos = -1;

        if (configLine.contains("APPLICATION")) {
            type = CNType.APPLICATION;
            regexTag = "APPLICATION";
        } else if (configLine.contains("MODULE")) {
            type = CNType.MODULE;
            regexTag = "MODULE";
        } else if (configLine.contains("FUNC")) {
            type = CNType.FUNCTION;
            regexTag = "FUNC";
        } else if (configLine.contains("BBLK")) {
            type = CNType.BASIC_BLOCK;
            regexTag = "BBLK";
        } else if (configLine.contains("INSN")) {
            type = CNType.INSTRUCTION;
            regexTag = "INSN";
        } else {
            type = CNType.NONE;
            regexTag = "";
        }

        if (configLine.length() > 1) {
            switch (configLine.charAt(1)) {
                case ' ':   status = CNStatus.NONE;      break;
                case 'i':   status = CNStatus.IGNORE;    break;
                case 's':   status = CNStatus.SINGLE;    break;
                case 'd':   status = CNStatus.DOUBLE;    break;
                case 'c':   status = CNStatus.CANDIDATE; candidate = true; break;
                default:    status = CNStatus.NONE;      break;
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
        regexTag += " #" + number;

        if (configLine.length() > 2) {
            pos = configLine.indexOf(':');
            if (pos >= 0 && pos < configLine.length()-1) {
                label = configLine.substring(pos+1).trim();
            }
        } else {
            label = "(empty)";
        }

        address = Util.extractRegex(configLine, "(0x[0-9a-fA-F]+)", 0);
        regexTag += ": " + address;

        insnCount = -1;
        resetExecCounts();
        error = 0.0;
        tested = false;
    }

    public void resetExecCounts() {
        insnExecCount = 0;
        totalExecCount = 0;
        execCount = new HashMap<CNStatus, Long>();
    }

    public CNStatus getEffectiveStatus() {
        if (getParent() != null) {
            TreeNode parent = getParent();
            if (parent instanceof ConfigTreeNode) {
                CNStatus parentStatus = ((ConfigTreeNode)parent).getEffectiveStatus();
                if (parentStatus != CNStatus.NONE) {
                    return parentStatus;
                }
            }
        }
        return status;
    }

    public long getInsnCount() {
        long count = 0;
        if (insnCount == -1) {
            switch (type) {
                case APPLICATION:
                case MODULE:
                case FUNCTION:
                case BASIC_BLOCK:
                    Enumeration<ConfigTreeNode> children = children();
                    while (children.hasMoreElements()) {
                        count += children.nextElement().getInsnCount();
                    }
                    break;
                case INSTRUCTION:
                    count = 1;
                    break;
            }
            insnCount = count;
        } else {
            count = insnCount;
        }
        return count;
    }

    public void toggleNoneSingle() {
        switch (status) {
            case NONE:      status = CNStatus.SINGLE;       break;
            case IGNORE:    status = CNStatus.NONE;         break;
            case SINGLE:    status = CNStatus.NONE;         break;
            case DOUBLE:    status = CNStatus.NONE;         break;
            case CANDIDATE: status = CNStatus.NONE;         break;
            default:        status = CNStatus.NONE;         break;
        }
    }

    public void toggleIgnoreSingle() {
        switch (status) {
            case NONE:      status = CNStatus.IGNORE;       break;
            case IGNORE:    status = CNStatus.SINGLE;       break;
            case SINGLE:    status = CNStatus.IGNORE;       break;
            case DOUBLE:    status = CNStatus.IGNORE;       break;
            case CANDIDATE: status = CNStatus.IGNORE;       break;
            default:        status = CNStatus.IGNORE;       break;
        }
    }

    public void toggleAll() {
        switch (status) {
            case NONE:      status = CNStatus.IGNORE;       break;
            case IGNORE:    status = CNStatus.SINGLE;       break;
            case SINGLE:    status = CNStatus.DOUBLE;       break;
            case DOUBLE:    status = CNStatus.NONE;         break;
            case CANDIDATE: status = CNStatus.NONE;         break;
            default:        status = CNStatus.NONE;         break;
        }
    }

    public void merge(ConfigTreeNode node) {
        //System.out.println("Merging: " + Status2Str(status) + "  " + label);
        //System.out.println("   with: " + Status2Str(node.status) + "  " + node.label);

        if (status == CNStatus.NONE || node.status == CNStatus.NONE) {
            status = CNStatus.NONE;
        } else if (status == CNStatus.IGNORE || node.status == CNStatus.IGNORE) {
            status = CNStatus.IGNORE;
        } else if (status == CNStatus.DOUBLE || node.status == CNStatus.DOUBLE) {
            status = CNStatus.DOUBLE;
        }
        
        if (status == CNStatus.CANDIDATE || node.status == CNStatus.CANDIDATE) {
            //System.out.println("Marking " + label + " as candidate!");
            candidate = true;  // don't overwrite other status with "candidate"
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
            case CANDIDATE: str.append('c');    break;
            default:        str.append(' ');    break;
        }
        str.append(' ');
        switch (type) {
            case APPLICATION:       str.append("APPLICATION");    break;
            case MODULE:            str.append("  MODULE");    break;
            case FUNCTION:          str.append("    FUNC");         break;
            case BASIC_BLOCK:       str.append("      BBLK");       break;
            case INSTRUCTION:       str.append("        INSN");     break;
            default:                str.append("UNKNOWN");        break;
        }
        str.append(" #");
        str.append(number);
        str.append(": ");
        str.append(label);
        return str.toString();
    }

    public void setError(double err) {
        error = err;
        Enumeration<ConfigTreeNode> childNodes = children();
        ConfigTreeNode child = null;
        while (childNodes.hasMoreElements()) {
            child = childNodes.nextElement();
            child.setError(err);
        }
    }

    public void setTested(boolean value) {
        tested = value;
    }

    public String toString() {
        return toString(false, false);
    }

    public String toString(boolean showCodeCoverage, boolean showError) {
        StringBuffer str = new StringBuffer();
        switch (type) {
            case APPLICATION:       str.append("APPLICATION");    break;
            case MODULE:            str.append("  MODULE");    break;
            case FUNCTION:          str.append("    FUNC");         break;
            case BASIC_BLOCK:       str.append("      BBLK");       break;
            case INSTRUCTION:       str.append("        INSN");     break;
            default:                str.append("UNKNOWN");        break;
        }
        //str.append(" #");
        //str.append(number);
        str.append(": ");
        str.append(label);
        if (type == CNType.MODULE || type == CNType.FUNCTION) {
            long coverage = 0;
            if (insnCount > 0) {
                coverage = insnExecCount * 100 / insnCount;
            }
            Long sgl = new Long(0), dbl = new Long(0), ign = new Long(0);
            Long gsgl = new Long(0), gdbl = new Long(0), gign = new Long(0);
            if (totalExecCount > 0) {
                if (execCount.containsKey(ConfigTreeNode.CNStatus.SINGLE)) {
                    sgl = execCount.get(ConfigTreeNode.CNStatus.SINGLE) * 100 / new Long(totalExecCount);
                }
                if (execCount.containsKey(ConfigTreeNode.CNStatus.DOUBLE)) {
                    dbl = execCount.get(ConfigTreeNode.CNStatus.DOUBLE) * 100 / new Long(totalExecCount);
                }
                if (execCount.containsKey(ConfigTreeNode.CNStatus.IGNORE)) {
                    ign = execCount.get(ConfigTreeNode.CNStatus.IGNORE) * 100 / new Long(totalExecCount);
                }
            }
            TreeNode root = this;
            while (root.getParent() != null) {
                root = root.getParent();
            }
            if (root instanceof ConfigTreeNode) {
                long rootExecCount = ((ConfigTreeNode)root).totalExecCount;
                if (rootExecCount > 0) {
                    if (execCount.containsKey(ConfigTreeNode.CNStatus.SINGLE)) {
                        gsgl = execCount.get(ConfigTreeNode.CNStatus.SINGLE) * 100 / new Long(rootExecCount);
                    }
                    if (execCount.containsKey(ConfigTreeNode.CNStatus.DOUBLE)) {
                        gdbl = execCount.get(ConfigTreeNode.CNStatus.DOUBLE) * 100 / new Long(rootExecCount);
                    }
                    if (execCount.containsKey(ConfigTreeNode.CNStatus.IGNORE)) {
                        gign = execCount.get(ConfigTreeNode.CNStatus.IGNORE) * 100 / new Long(rootExecCount);
                    }
                }
            }

            while (str.length() < 50) {
                str.append(' ');
            }
            if (type == CNType.MODULE) {
                str.append("   ");
            }
            str.append(" [");
            str.append(getInsnCount());
            str.append(" instruction(s)]");

            if (showCodeCoverage) {
                while (str.length() < 80) {
                    str.append(' ');
                }
                if (type == CNType.MODULE) {
                    str.append("   ");
                }
                str.append("  [global: ");
                str.append(String.format("%3d", gsgl));
                str.append("%/");
                str.append(String.format("%3d", gdbl));
                str.append("%/");
                str.append(String.format("%3d", gign));
                str.append("; local: ");
                str.append(String.format("%3d", coverage));
                str.append("% cov, ");
                str.append(String.format("%3d", sgl));
                str.append("%/");
                str.append(String.format("%3d", dbl));
                str.append("%/");
                str.append(String.format("%3d", ign));
                str.append("% of ");
                str.append(totalExecCount);
                //str.append(" execution(s)");
                str.append("]");
            }
        } else if (type == CNType.INSTRUCTION) {
            while (str.length() < 50) {
                str.append(' ');
            }
            str.append(" [");
            str.append(totalExecCount);
            str.append(" execution(s)");
            str.append("]");
        }

        if (showError) {
            str.append("  Err=");
            str.append(error);
        }
        
        return str.toString();
    }

}

