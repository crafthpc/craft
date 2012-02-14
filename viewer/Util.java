/**
 * Util
 *
 * misc utility methods
 */

import java.io.*;
import java.text.*;
import java.util.*;

public class Util {

    private static List<File> searchDirs = new ArrayList<File>();

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

    public static List<File> getSearchDirs() {
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
}

