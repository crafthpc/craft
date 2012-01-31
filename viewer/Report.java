/**
 * Report
 *
 * generic report that can be run on log files
 */

import java.awt.*;

public interface Report {

    public void runReport(LLogFile log);

    public Component generateUI();

    public String getTitle();
    public String getText();

}


