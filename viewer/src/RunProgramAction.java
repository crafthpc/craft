/**
 * RunProgramAction
 *
 * try to run the profiler on a program and display the results
 * (doesn't currently work!)
 */

import java.io.*;
import java.awt.event.*;
import javax.swing.*;

public class RunProgramAction extends AbstractAction {

    public static final String PROFILER_PATH = "/fs/dyninst/lam/git-fpinst/fprofiler";

    ViewerApp appInst;

    public RunProgramAction(ViewerApp app, String text, ImageIcon icon, Integer mnemonic) {
        super(text, icon);
        appInst = app;
        putValue(MNEMONIC_KEY, mnemonic);
    }

    public void actionPerformed(ActionEvent e) {
        int returnVal;
        appInst.fileChooser.setFileFilter(new ProgFilter());
        returnVal = appInst.fileChooser.showOpenDialog(appInst);

        if (returnVal == JFileChooser.APPROVE_OPTION) {
            File file = appInst.fileChooser.getSelectedFile();
            try {
                String mutatee = file.getAbsolutePath();
                String command;
                String[] env = {
                    "DYNINST_ROOT=/fs/dyninst/lam/git-fpinst",
                    "LD_LIBRARY_PATH=/fs/dyninst/lam/git-fpinst/i386-unknown-linux2.4/lib:/fs/maxfli/lam/lib:/fs/mashie/lam/local/lib:.",
                    "DYNINSTAPI_RT_LIB=/fs/dyninst/lam/git-fpinst/i386-unknown-linux2.4/lib/libdyninstAPI_RT.so.1"};
                Process proc;

                command = PROFILER_PATH + "/fprofiler -o " + mutatee + ".mutant " + mutatee;
                JOptionPane.showMessageDialog(null, command);
                proc = Runtime.getRuntime().exec(command, env);

                //command = "/bin/bash " + PROFILER_PATH + "/fpcancel " + mutatee;
                //JOptionPane.showMessageDialog(null, command);
                //Runtime.getRuntime().exec(command).waitFor();
                
                InputStream inputstream =
                    proc.getErrorStream();
                InputStreamReader inputstreamreader =
                    new InputStreamReader(inputstream);
                BufferedReader bufferedreader =
                    new BufferedReader(inputstreamreader);
                String line;
                while ((line = bufferedreader.readLine()) 
                          != null) {
                    System.out.println(line);
                }
                try {
                    if (proc.waitFor() != 0) {
                        System.err.println("exit value = " +
                            proc.exitValue());
                    }
                }
                catch (InterruptedException ex) {
                    System.err.println(ex);
                }
                bufferedreader.close();

                //command = "\nLOGFILE=" + mutatee + ".log " + mutatee + ".mutant";
                //JOptionPane.showMessageDialog(null, command);
                //Runtime.getRuntime().exec(command).waitFor();


                //command = "cat /etc/passwd >/tmp/tempfile";
                //Runtime.getRuntime().exec(command).waitFor();

                //appInst.openLogFile(new File(mutatee + ".log"));
            }
            catch (Exception ex) {
                JOptionPane.showMessageDialog(null, ex.getMessage());
            }
        }
    }

}

