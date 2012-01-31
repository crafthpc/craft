/**
 * Util
 *
 * misc utility methods
 */

import java.text.*;
import java.util.*;

public class Util {

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

