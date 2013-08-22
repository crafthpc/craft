/**
 * ConfigReport
 *
 * generic report that can be run on config trees
 */

import java.awt.*;

public interface ConfigReport {

    public void runReport(ConfigTreeNode node);

    public Component generateUI();

    public String getTitle();
    public String getText();

}


