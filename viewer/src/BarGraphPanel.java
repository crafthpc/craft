/**
 * BarGraphPanel
 */

import java.awt.*;
import java.awt.event.*;
import java.math.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.*;
import javax.swing.border.*;

public class BarGraphPanel extends JPanel {
    
    private long[] values;
    private String[] labels;

    private final static float DASH_1[] = {2.0f};
    private final static BasicStroke DASHED_STROKE = new BasicStroke(2.0f,
                        BasicStroke.CAP_BUTT, BasicStroke.JOIN_MITER,
                        10.0f, DASH_1, 0.0f);

    public BarGraphPanel(long[] values, String[] labels) {
        assert(values.length == labels.length);
        this.values = values;
        this.labels = labels;
    }

    public void paint(Graphics g) {

        Graphics2D g2 = (Graphics2D)g;

        // find max value (for scaling)
        long max = 0;
        for (int i=0; i<values.length; i++) {
            if (values[i] > max) {
                max = values[i];
            }
        }

        // format max value (for label)
        float nmax = (float)max;
        String tag = "";
        if (nmax > 1024.0) { nmax /= 1024.0; tag = "K"; }
        if (nmax > 1024.0) { nmax /= 1024.0; tag = "M"; }
        if (nmax > 1024.0) { nmax /= 1024.0; tag = "G"; }
        String botLabel = "0";
        String topLabel = ""+max;
        if (!tag.equals("")) {
            topLabel = String.format("%.1f%s", nmax, tag);
        }

        // get draw limits
        Rectangle bounds = g.getClipBounds();

        // fill background (before border adjustment)
        g.setColor(Color.WHITE);
        g.fillRect(bounds.x, bounds.y, bounds.width, bounds.height);
        g.setColor(Color.BLACK);
        g.drawRect(bounds.x, bounds.y, bounds.width, bounds.height);

        // adjust for border whitespace
        int borderSize = (int)(0.075*(float)Math.min(bounds.width, bounds.height));
        bounds.setLocation(bounds.x+borderSize, bounds.y+borderSize);
        bounds.setSize(bounds.width-borderSize*2, bounds.height-borderSize*2);

        // adjust for axes
        int leftAxisWidth = (int)(0.10 * (float)bounds.width);
        int bottomAxisHeight = (int)(0.10 * (float)bounds.height);
        Rectangle graphBounds = new Rectangle(
                bounds.x+leftAxisWidth,
                bounds.y,
                bounds.width-leftAxisWidth,
                bounds.height-bottomAxisHeight);

        // calculate bar metrics
        int barWidth = graphBounds.width / (values.length+1);
        int firstBarBuffer = (int)(1.0*(float)barWidth);

        // set font
        g.setFont(ConfigEditorApp.DEFAULT_FONT_MONO_PLAIN);
        FontMetrics fm = getFontMetrics(g.getFont());

        // draw left axis
        g.setColor(Color.BLACK);
        g.drawLine(graphBounds.x,
                   graphBounds.y,
                   graphBounds.x,
                   graphBounds.y+graphBounds.height);
        g.drawString(botLabel,
                graphBounds.x-fm.stringWidth(botLabel)-(int)(0.1*(float)leftAxisWidth),
                graphBounds.y+graphBounds.height);
        g.drawString(topLabel,
                graphBounds.x-fm.stringWidth(topLabel)-(int)(0.1*(float)leftAxisWidth),
                graphBounds.y+fm.getHeight());
        
        // draw bottom axis
        g.setColor(Color.BLACK);
        g.drawLine(graphBounds.x,
                   graphBounds.y+graphBounds.height,
                   graphBounds.x+graphBounds.width,
                   graphBounds.y+graphBounds.height);
        int interval = 1;
        if (values.length > 10) { interval = 2; }
        if (values.length > 20) { interval = 5; }
        for (int i=0; i<values.length; i += interval) {
            int x = graphBounds.x+firstBarBuffer+i*barWidth;
            x += barWidth/2 - fm.stringWidth(labels[i])/2;
            g.drawString(labels[i], x,
                    graphBounds.y+graphBounds.height+bottomAxisHeight/2+fm.getHeight()/2);
        }

        // draw each bar
        for (int i=0; i<values.length; i++) {
            float pct = (float)values[i] / (float)max;
            int height = (int)(pct * (float)graphBounds.height);

            g.setColor(Color.BLACK);
            g.fillRect(
                    graphBounds.x+firstBarBuffer+i*barWidth,
                    graphBounds.y+graphBounds.height-height,
                    barWidth,
                    height);

            // red dashed line for single-precision
            if (labels[i].startsWith("23")) {
                g.setColor(Color.RED);
                Stroke oldStroke = g2.getStroke();
                g2.setStroke(DASHED_STROKE);
                g.drawLine(
                        graphBounds.x+firstBarBuffer+i*barWidth,
                        graphBounds.y,
                        graphBounds.x+firstBarBuffer+i*barWidth,
                        graphBounds.y+graphBounds.height);
                g2.setStroke(oldStroke);
            }
        }
    }
}

