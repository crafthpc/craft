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
        NONE, IGNORE, CANDIDATE, SINGLE, DOUBLE,
        TRANGE, NULL, CINST, DCANCEL, DNAN
    }

    public static String type2Str(CNType type) {
        String str = "UNKNOWN";
        switch (type) {
            case APPLICATION:       str = "APPL";    break;
            case MODULE:            str = "MODL";    break;
            case FUNCTION:          str = "FUNC";    break;
            case BASIC_BLOCK:       str = "BBLK";    break;
            case INSTRUCTION:       str = "INSN";    break;
            default:                                 break;
        }
        return str;
    }

    public static String status2Str(CNStatus status) {
        String str = "UNKNOWN";
        switch (status) {
            case NONE:      str = "NONE";    break;
            case IGNORE:    str = "IGNR";    break;
            case CANDIDATE: str = "CAND";    break;
            case SINGLE:    str = "SING";    break;
            case DOUBLE:    str = "DOUB";    break;
            case TRANGE:    str = "TRAN";    break;
            case NULL:      str = "NULL";    break;
            case CINST:     str = "CINS";    break;
            case DCANCEL:   str = "DCAN";    break;
            case DNAN:      str = "DNAN";    break;
            default:                         break;
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
                case '!':   status = CNStatus.IGNORE;    break;
                case '?':   status = CNStatus.CANDIDATE; candidate = true; break;
                case 's':   status = CNStatus.SINGLE;    break;
                case 'd':   status = CNStatus.DOUBLE;    break;
                case 'r':   status = CNStatus.TRANGE;    break;
                case 'x':   status = CNStatus.NULL;      break;
                case 'i':   status = CNStatus.CINST;     break;
                case 'c':   status = CNStatus.DCANCEL;   break;
                case 'n':   status = CNStatus.DNAN;      break;
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
        CNStatus fromParent = getEffectiveStatusFromParent();
        if (fromParent != status) {
            return fromParent;
        }
        CNStatus fromChildren = getEffectiveStatusFromChildren();
        if (fromChildren != status) {
            return fromChildren;
        }
        return status;
    }

    public CNStatus getEffectiveStatusFromParent() {
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

    public CNStatus getEffectiveStatusFromChildren() {
        /* TODO: optimize this by caching; right now this is very inefficient
         * since it traverses the tree excessively */
        if (getChildCount() > 0) {
            Enumeration children = children();
            CNStatus effectiveStatus = ((ConfigTreeNode)children.nextElement()).getEffectiveStatusFromChildren();
            while (children.hasMoreElements()) {
                CNStatus tempStatus = ((ConfigTreeNode)children.nextElement()).getEffectiveStatusFromChildren();
                if (tempStatus != effectiveStatus) {
                    return status;
                }
            }
            return effectiveStatus;
        } else {
            return status;
        }
    }

    public void resetInsnCount() {
        insnCount = -1;
    }

    public long getInsnCount() {
        long count = 0;
        if (insnCount == -1) {
            switch (type) {
                case APPLICATION:
                case MODULE:
                case FUNCTION:
                case BASIC_BLOCK:
                    Enumeration children = children();
                    while (children.hasMoreElements()) {
                        count += ((ConfigTreeNode)children.nextElement()).getInsnCount();
                    }
                    break;
                case INSTRUCTION:
                    count = 1;
                    break;
                default:
                    break;
            }
            insnCount = count;
        } else {
            count = insnCount;
        }
        return count;
    }

    public void toggleNoneSingle() {
        if (status == CNStatus.NONE) {
            status = CNStatus.SINGLE;
        } else {
            status = CNStatus.NONE;
        }
    }

    public void toggleIgnoreSingle() {
        if (status == CNStatus.IGNORE) {
            status = CNStatus.SINGLE;
        } else {
            status = CNStatus.IGNORE;
        }
    }

    public void toggleAll() {
        switch (status) {
            case NONE:    /*status = CNStatus.IGNORE;       break;  // these have their own button
            case IGNORE:    status = CNStatus.CANDIDATE;    break;
            case CANDIDATE: status = CNStatus.SINGLE;       break;
            case SINGLE:    status = CNStatus.DOUBLE;       break;
            case DOUBLE:    status = CNStatus.NULL;         break;
            case NULL:    */status = CNStatus.TRANGE;       break;
            case TRANGE:    status = CNStatus.CINST;        break;
            case CINST:     status = CNStatus.DCANCEL;      break;
            case DCANCEL:   status = CNStatus.DNAN;         break;
            case DNAN:      status = CNStatus.NONE;         break;
            default:        status = CNStatus.NONE;         break;
        }
    }

    public void merge(ConfigTreeNode node) {
        //System.out.println("Merging: " + status2Str(status) + "  " + label);
        //System.out.println("   with: " + status2Str(node.status) + "  " + node.label);

        if (candidate && node.status != CNStatus.CANDIDATE) {
            // set new status (remains tagged as candidate)
            status = node.status;
        } else if (node.status == CNStatus.CANDIDATE) {
            // tag as candidate
            candidate = true;
        } else if (status == CNStatus.IGNORE || node.status == CNStatus.IGNORE) {
            status = CNStatus.IGNORE;
        } else if (status == CNStatus.DOUBLE || node.status == CNStatus.DOUBLE) {
            status = CNStatus.DOUBLE;
        } else if (status == CNStatus.SINGLE || node.status == CNStatus.SINGLE) {
            status = CNStatus.SINGLE;
        } else if (status == CNStatus.TRANGE || node.status == CNStatus.TRANGE) {
            status = CNStatus.TRANGE;
        } else if (status == CNStatus.CINST || node.status == CNStatus.CINST) {
            status = CNStatus.CINST;
        } else if (status == CNStatus.DCANCEL || node.status == CNStatus.DCANCEL) {
            status = CNStatus.DCANCEL;
        } else if (status == CNStatus.DNAN || node.status == CNStatus.DNAN) {
            status = CNStatus.DNAN;
        } else if (status == CNStatus.NULL || node.status == CNStatus.NULL) {
            status = CNStatus.NULL;
        } // only other status is NONE
    }

    public String formatFileEntry() {
        StringBuffer str = new StringBuffer();
        str.append('^');
        switch (status) {
            case NONE:      str.append(' ');    break;
            case IGNORE:    str.append('!');    break;
            case CANDIDATE: str.append('?');    break;
            case SINGLE:    str.append('s');    break;
            case DOUBLE:    str.append('d');    break;
            case TRANGE:    str.append('r');    break;
            case NULL:      str.append('x');    break;
            case CINST:     str.append('i');    break;
            case DCANCEL:   str.append('c');    break;
            case DNAN:      str.append('n');    break;
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
        Enumeration childNodes = children();
        ConfigTreeNode child = null;
        while (childNodes.hasMoreElements()) {
            child = (ConfigTreeNode)childNodes.nextElement();
            child.setError(err);
        }
    }

    public void setTested(boolean value) {
        tested = value;
    }

    public void preManipulate(ConfigTreeIterator iterator) {
        Enumeration children = children();
        ConfigTreeNode child = null;
        iterator.manipulate(this);
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
            child.preManipulate(iterator);
        }
    }

    public void postManipulate(ConfigTreeIterator iterator) {
        Enumeration children = children();
        ConfigTreeNode child = null;
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
            child.postManipulate(iterator);
        }
        iterator.manipulate(this);
    }

    public void getRegexTagLookups(Map<String,ConfigTreeNode> lookup) {
        lookup.put(regexTag, this);
        Enumeration children = children();
        ConfigTreeNode child = null;
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
            child.getRegexTagLookups(lookup);
        }
    }

    public void getStatusCounts(Map<CNStatus,Long> counts) {
        Enumeration children = children();
        ConfigTreeNode child = null;
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
            child.getStatusCounts(counts);
        }
        if (type == CNType.INSTRUCTION) {
            Long count = Long.valueOf(1);
            if (counts.containsKey(status)) {
                count = Long.valueOf(1 + counts.get(status).longValue());
            }
            counts.put(status, count);
        }
    }

    public void updateExecCounts(LLogFile logfile) {
        if (type == ConfigTreeNode.CNType.INSTRUCTION) {
            CNStatus status = getEffectiveStatus();
            if (logfile != null) {
                // read exec count from log file if present
                LInstruction insn = logfile.instructionsByAddress.get(address);
                if (insn != null) {
                    totalExecCount = insn.count;
                }
            }
            if (totalExecCount > 0) {
                insnExecCount = 1;      // code coverage
            }
            execCount.put(status, Long.valueOf(totalExecCount));
        } else {
            Enumeration children = children();
            ConfigTreeNode child = null;
            resetExecCounts();
            while (children.hasMoreElements()) {

                // recurse
                child = (ConfigTreeNode)children.nextElement();
                child.updateExecCounts(logfile);

                // update aggregate counts
                insnExecCount  += child.insnExecCount;
                totalExecCount += child.totalExecCount;
                for (Map.Entry<CNStatus,Long> entry : child.execCount.entrySet()) {
                    long newCount = entry.getValue().longValue();
                    if (execCount.containsKey(entry.getKey())) {
                        newCount += execCount.get(entry.getKey()).longValue();
                    }
                    execCount.put(entry.getKey(), Long.valueOf(newCount));
                }
            }
        }
    }

    public void batchConfig(CNStatus orig, CNStatus dest) {
        Enumeration children = children();
        ConfigTreeNode child = null;
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
            child.batchConfig(orig, dest);
        }
        if (type == CNType.INSTRUCTION && status == orig) {
            status = dest;
        }
    }

    public void removeChildren(ConfigTreeIterator iterator) {
        Set<ConfigTreeNode> nodesToRemove = new HashSet<ConfigTreeNode>();
        Enumeration children = children();
        ConfigTreeNode child = null;
        while (children.hasMoreElements()) {
            child = (ConfigTreeNode)children.nextElement();
            child.removeChildren(iterator);
            if (iterator.test(child)) {
                nodesToRemove.add(child);
            }
        }
        if (nodesToRemove.size() > 0) {
            resetInsnCount();
            for (ConfigTreeNode node : nodesToRemove) {
                remove(node);
            }
        }
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
            Long sgl = Long.valueOf(0), dbl = Long.valueOf(0), ign = Long.valueOf(0);
            Long gsgl = Long.valueOf(0), gdbl = Long.valueOf(0), gign = Long.valueOf(0);
            if (totalExecCount > 0) {
                if (execCount.containsKey(ConfigTreeNode.CNStatus.SINGLE)) {
                    sgl = execCount.get(ConfigTreeNode.CNStatus.SINGLE) * 100 / totalExecCount;
                }
                if (execCount.containsKey(ConfigTreeNode.CNStatus.DOUBLE)) {
                    dbl = execCount.get(ConfigTreeNode.CNStatus.DOUBLE) * 100 / totalExecCount;
                }
                if (execCount.containsKey(ConfigTreeNode.CNStatus.IGNORE)) {
                    ign = execCount.get(ConfigTreeNode.CNStatus.IGNORE) * 100 / totalExecCount;
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
                        gsgl = execCount.get(ConfigTreeNode.CNStatus.SINGLE) * 100 / rootExecCount;
                    }
                    if (execCount.containsKey(ConfigTreeNode.CNStatus.DOUBLE)) {
                        gdbl = execCount.get(ConfigTreeNode.CNStatus.DOUBLE) * 100 / rootExecCount;
                    }
                    if (execCount.containsKey(ConfigTreeNode.CNStatus.IGNORE)) {
                        gign = execCount.get(ConfigTreeNode.CNStatus.IGNORE) * 100 / rootExecCount;
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
        } else if (type == CNType.INSTRUCTION && showCodeCoverage) {
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

