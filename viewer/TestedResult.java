/**
 * TestedResult.java
 *
 * Stores results from a single test performed by the search script.
 *
 * Original author:
 *   Mike Lam (lam@cs.umd.edu)
 *   Professor Jeffrey K. Hollingsworth, UMD
 *   February 2013
 */

import java.util.*;

public class TestedResult {

    public String cuid;
    public String default_cfg;
    public String label;

    public String level;
    public String result;
    public long runtime;
    public double error;

    public Map<String, String> exceptions;

    public TestedResult() {
        cuid = "";
        default_cfg = "";
        label = "";
        level = "";
        result = "";
        runtime = 0;
        error = 0.0;
        exceptions = new HashMap<String,String>();
    }

}

