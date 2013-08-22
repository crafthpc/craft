/**
 * SourceFileInfo.java
 *
 * Stores debug file information for the config viewer.
 */

import java.util.*;

public class SourceFileInfo {

    public String filename;     // filename (no path; key for allFiles)
    public String path;         // full file path

    // line_number => replacement count mappings
    public Map<Integer, Integer> singleCounts;
    public Map<Integer, Integer> doubleCounts;

    // line_number => maximum necessary precision mapping
    public Map<Integer, Long> maxPrecisions;

    // aggregate replacement counts
    public int totalSingleCount;
    public int totalDoubleCount;

    // aggregate max precision
    public long overallMaxPrecision;

    // for drawing border
    public int maxLineLength;

    public SourceFileInfo(String fn) {
        filename = fn;
        path = null;
        singleCounts = new HashMap<Integer, Integer>();
        doubleCounts = new HashMap<Integer, Integer>();
        maxPrecisions = new HashMap<Integer, Long>();
        totalSingleCount = 0;
        totalDoubleCount = 0;
        overallMaxPrecision = 0;
        maxLineLength = 0;
    }

    public void incrementSingle(int lineno) {
        Integer i = Integer.valueOf(lineno);
        if (singleCounts.containsKey(i)) {
            singleCounts.put(i, Integer.valueOf(singleCounts.get(i).intValue()+1));
        } else {
            singleCounts.put(i, Integer.valueOf(1));
        }
        totalSingleCount++;
        int thisLineLength = singleCounts.get(i).intValue();
        if (doubleCounts.containsKey(i)) {
            thisLineLength += doubleCounts.get(i).intValue();
        }
        if (thisLineLength > maxLineLength) {
            maxLineLength = thisLineLength;
        }
    }

    public void incrementDouble(int lineno) {
        Integer i = Integer.valueOf(lineno);
        if (doubleCounts.containsKey(i)) {
            doubleCounts.put(i, Integer.valueOf(doubleCounts.get(i).intValue()+1));
        } else {
            doubleCounts.put(i, Integer.valueOf(1));
        }
        totalDoubleCount++;
        int thisLineLength = doubleCounts.get(i).intValue();
        if (singleCounts.containsKey(i)) {
            thisLineLength += singleCounts.get(i).intValue();
        }
        if (thisLineLength > maxLineLength) {
            maxLineLength = thisLineLength;
        }
    }

    public void setMaxPrecision(int lineno, long precision) {
        Integer i = Integer.valueOf(lineno);
        if (maxPrecisions.containsKey(i)) {
            maxPrecisions.put(i, Long.valueOf(Math.max(maxPrecisions.get(i).longValue(), precision)));
        } else {
            maxPrecisions.put(i, Long.valueOf(precision));
        }
        overallMaxPrecision = Math.max(overallMaxPrecision, precision);
        maxLineLength = Math.max(3, maxLineLength);
    }

    public String toString() {
        // pretty display for the file list box
        // (includes total replacement counts)
        String label = filename;
        if (totalSingleCount > 0) {
            label += "  S:" + totalSingleCount;
        }
        if (totalDoubleCount > 0) {
            label += "  D:" + totalDoubleCount;
        }
        if (overallMaxPrecision > 0) {
            label += "  P:" + overallMaxPrecision;
        }
        return label;
    }
}

