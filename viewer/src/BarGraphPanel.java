/**
 * BarGraphPanel
 */

import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.math.*;
import java.util.*;
import java.util.regex.*;
import javax.imageio.*;
import javax.swing.*;
import javax.swing.border.*;

public class BarGraphPanel extends JPanel implements MouseListener {
    
    private static final Font AXIS_FONT = new Font("Monospaced", Font.PLAIN, 20);

    private long[] values;
    private String[] labels;

    private final static float DASH_1[] = {2.0f};
    private final static BasicStroke DASHED_STROKE = new BasicStroke(2.0f,
                        BasicStroke.CAP_BUTT, BasicStroke.JOIN_MITER,
                        10.0f, DASH_1, 0.0f);

    private int yLabelCount;
    private boolean showRawYAxis = false;

    public BarGraphPanel(long[] values, String[] labels) {
        assert(values.length == labels.length);
        this.values = values;
        this.labels = labels;
        yLabelCount = 5;
        addMouseListener(this);
    }

    public void setShowRawYAxis(boolean show) {
        showRawYAxis = show;
    }

    public void paint(Graphics g) {

        Graphics2D g2 = (Graphics2D)g;

        // find max value (for scaling)
        // and total count (for y-axis labeling)
        long max = 0;
        long sum = 0;
        for (int i=0; i<values.length; i++) {
            if (values[i] > max) {
                max = values[i];
            }
            sum += values[i];
        }

        // format y-axis labels
        String[] ylabels = new String[yLabelCount];
        for (int i=0; i<yLabelCount; i++) {
            float yval = (float)max * ((float)i / (float)(yLabelCount-1));
            if (!showRawYAxis) {
                yval /= (float)sum;
                yval *= 100.0;
            }
            String tag = showRawYAxis ? "" : "%";
            if (yval > 1024.0) { yval /= 1024.0; tag = "K"; }
            if (yval > 1024.0) { yval /= 1024.0; tag = "M"; }
            if (yval > 1024.0) { yval /= 1024.0; tag = "G"; }
            String ylbl = String.format("%.1f%s", yval, tag);
            ylabels[i] = ylbl;
        }

        // get draw limits
        Rectangle bounds = g.getClipBounds();

        // fill background (before border adjustment)
        g.setColor(Color.WHITE);
        g.fillRect(bounds.x, bounds.y, bounds.width, bounds.height);
        g.setColor(Color.BLACK);
        g.drawRect(bounds.x, bounds.y, bounds.width-1, bounds.height-1);

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
        g.setFont(AXIS_FONT);
        FontMetrics fm = getFontMetrics(g.getFont());

        // draw left axis
        g.setColor(Color.BLACK);
        g.drawLine(graphBounds.x,
                   graphBounds.y,
                   graphBounds.x,
                   graphBounds.y+graphBounds.height);
        for (int i=0; i<yLabelCount; i++) {
            int offset = (int)( ((float)i / (float)(yLabelCount-1)) *
                                (float)(graphBounds.height-fm.getHeight()) );
            g.drawString(ylabels[i],
                    graphBounds.x-fm.stringWidth(ylabels[i])-(int)(0.1*(float)leftAxisWidth),
                    graphBounds.y+graphBounds.height-offset);
        }
        
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

    public void saveImageToFile(File fout) {
        saveImageToFile(fout, false);
    }

    public void saveImageToFile(File fout, boolean silent) {
        final int IMG_WIDTH  =  800;
        final int IMG_HEIGHT =  700;
        BufferedImage img = new BufferedImage(IMG_WIDTH, IMG_HEIGHT,
                BufferedImage.TYPE_INT_RGB);
        Graphics2D g2 = img.createGraphics();
        g2.setClip(0, 0, IMG_WIDTH, IMG_HEIGHT);
        paint(g2);
        try {
            if (ImageIO.write(img, "png", fout) && !silent) {
                JOptionPane.showMessageDialog(getParent(),
                        "Image saved!", "Success",
                        JOptionPane.INFORMATION_MESSAGE);
            }
        } catch (IOException io) {
            JOptionPane.showMessageDialog(getParent(),
                    "Error: " + io.getMessage(), "Error",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    public void saveImage() {
        JFileChooser chooser = new JFileChooser();
        ImageFilter filter = new ImageFilter();
        chooser.setCurrentDirectory(new File("."));
        chooser.setSelectedFile(new File("./rprec_graph.png"));
        chooser.setFileFilter(filter);
        int rval = chooser.showSaveDialog(getParent());
        if (rval == JFileChooser.APPROVE_OPTION) {
            saveImageToFile(chooser.getSelectedFile());
        }
    }

    public void mousePressed(MouseEvent e) { }
    public void mouseReleased(MouseEvent e) { }
    public void mouseEntered(MouseEvent e) { }
    public void mouseExited(MouseEvent e) { }

    public void mouseClicked(MouseEvent e) {
        saveImage();
    }
}

