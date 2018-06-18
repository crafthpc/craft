/**
 * Util
 *
 * misc utility methods
 */

import java.awt.*;
import java.io.*;
import java.text.*;
import java.util.*;
import java.util.regex.*;

public class Util {

    private static java.util.List<File> searchDirs = new ArrayList<File>();

    public static void initSearchDirs() {
        searchDirs = new ArrayList<File>();

        File dirList = new File("searchdirs.txt");
        if (dirList.exists()) {
            try {
                BufferedReader rdr = new BufferedReader(new FileReader(dirList));
                String dir;
                while ((dir = rdr.readLine()) != null) {
                    searchDirs.add(0, new File(dir));
                }
                rdr.close();
            } catch (IOException ex) { }
        }

        searchDirs.add(new File("."));
    }

    public static void addSearchDir(File path) {
        if (path != null) {
            searchDirs.add(0,path);
        }
    }

    public static java.util.List<File> getSearchDirs() {
        return searchDirs;
    }

    public static String findFile(File start, String name) {
        File[] files = start.listFiles();
        String ret = null;
        int i;
        for (i = 0; files != null && i < files.length && ret == null; i++) {
            if (files[i].isDirectory()) {
                ret = findFile(files[i], name);
            } else if (files[i].getName().equals(name)) {
                ret = files[i].getAbsolutePath();
            }
        }
        return ret;
    }

    public static String getExtension(File f) {
        String ext = null;
        String s = f.getName();
        int i = s.lastIndexOf('.');
        if (i > 0 &&  i < s.length() - 1) {
            ext = s.substring(i+1).toLowerCase();
        }
        return ext;
    }

    public static double parseDouble(String str) {
        if (str.length() >= 3 && str.substring(0,3).equalsIgnoreCase("inf")) {
            return Double.POSITIVE_INFINITY;
        } else if (str.length() >= 4 && str.charAt(0) == '-' && 
                str.substring(1,4).equalsIgnoreCase("inf")) {
            return Double.NEGATIVE_INFINITY;
        } else if (str.length() >= 3 && str.substring(0,3).equalsIgnoreCase("nan")) {
            return Double.NaN;
        } else {
            return Double.parseDouble(str);
        }
    }

    public static String extractRegex(String data, String regex, int group) {
        Pattern p = Pattern.compile(regex);
        Matcher m = p.matcher(data);
        String s = null;
        if (m.find()) {
            s = m.group(group);
        }
        return s;
    }

    public static Color getGreenRedScaledColor(float value) {
        value = Math.min(Math.max(value, 0.0f), 1.0f);
        float hue = (1.0f-value) * (115.0f / 360.0f);
        return Color.getHSBColor(hue, 0.35f, 1.0f);
    }

    public static Color getWhiteBlueScaledColor(float value) {
        value = Math.min(Math.max(value, 0.0f), 1.0f);
        float sat = value * 0.25f;
        return Color.getHSBColor((180.0f / 360.0f), sat, 1.0f);
    }

    public static Color getRedPurpleScaledColor(float value) {
        value = Math.min(Math.max(value, 0.0f), 1.0f);
        float hue = ((1.0f-value) * (110.0f/360.0f)) + (250.0f/360.0f);
        return Color.getHSBColor(hue, 0.35f, 1.0f);
    }

    public static Color getOverDoubleColor() {
        return Color.getHSBColor(250.0f/360.0f, 0.35f, 1.0f);
    }

    public static Color getPrecisionScaledColor(long precision) {
        Color c = Color.RED;
        if (precision > 64) {
            c = getOverDoubleColor();
        } else if (precision > 52 && precision <= 64) {
            float prec = ((float)precision-52.0f)/(64.0f-52.0f);
            c = getRedPurpleScaledColor(prec);
        } else if (precision > 23 && precision <= 52) {
            float prec = ((float)precision-23.0f)/(52.0f-23.0f);
            c = getGreenRedScaledColor(prec);
        } else if (precision <= 23) {
            float prec = ((float)precision)/(23.0f);
            c = getWhiteBlueScaledColor(prec);
        }
        return c;
    }
}

