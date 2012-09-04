/**
 * LogFileHandler
 *
 * handle XML parsing and structure initialization to load a logfile
 */

import java.io.*;
import java.util.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class LogFileHandler extends DefaultHandler {

    private Map<String, Boolean> parseStatus;
    private LLogFile logfile;
    private LMessage currentMessage;
    private LTrace currentTrace;
    private LFrame currentFrame;
    private LInstruction currentInstruction;
    private int initialTime = 0;

    public LogFileHandler() {
        parseStatus = new HashMap<String, Boolean>();
        parseStatus.put("log", Boolean.FALSE);
        parseStatus.put("app", Boolean.FALSE);
        parseStatus.put("message", Boolean.FALSE);
        parseStatus.put("time", Boolean.FALSE);
        parseStatus.put("priority", Boolean.FALSE);
        parseStatus.put("type", Boolean.FALSE);
        parseStatus.put("trace_id", Boolean.FALSE);
        parseStatus.put("inst_id", Boolean.FALSE);
        parseStatus.put("label", Boolean.FALSE);
        parseStatus.put("details", Boolean.FALSE);
        parseStatus.put("instruction", Boolean.FALSE);
        parseStatus.put("trace", Boolean.FALSE);
        parseStatus.put("id", Boolean.FALSE);
        parseStatus.put("frame", Boolean.FALSE);
        parseStatus.put("level", Boolean.FALSE);
        parseStatus.put("address", Boolean.FALSE);
        parseStatus.put("disassembly", Boolean.FALSE);
        parseStatus.put("function", Boolean.FALSE);
        parseStatus.put("file", Boolean.FALSE);
        parseStatus.put("lineno", Boolean.FALSE);
        parseStatus.put("module", Boolean.FALSE);
        parseStatus.put("text", Boolean.FALSE);
    }

    public LLogFile getLogFile() {
        return logfile;
    }

    public void startElement(String namespaceURI, String localName, 
            String qualifiedName, Attributes atts) throws SAXException {
        String temp;
        parseStatus.put(localName, Boolean.TRUE);
        if (localName.equals("log")) {
            logfile = new LLogFile();
            currentMessage = null;
            currentTrace = null;
            currentFrame = null;
            currentInstruction = null;
            temp = atts.getValue("app");
            if (temp != null) logfile.appname = temp;
        } else if (localName.equals("message")) {
            currentMessage = new LMessage();
            temp = atts.getValue("time");
            if (temp != null) currentMessage.time = temp;
            temp = atts.getValue("priority");
            if (temp != null) currentMessage.priority = temp;
            temp = atts.getValue("type");
            if (temp != null) currentMessage.type = temp;
            temp = atts.getValue("trace_id");
            if (temp != null) currentMessage.backtraceID = temp;
            temp = atts.getValue("inst_id");
            if (temp != null) currentMessage.instructionID = temp;
            temp = atts.getValue("label");
            if (temp != null) currentMessage.label = temp;
            temp = atts.getValue("details");
            if (temp != null) currentMessage.details = temp;
        } else if (localName.equals("trace")) {
            currentTrace = new LTrace();
            temp = atts.getValue("id");
            if (temp != null) currentTrace.id = temp;
        } else if (localName.equals("frame")) {
            currentFrame = new LFrame();
            temp = atts.getValue("level");
            if (temp != null) currentFrame.level = temp;
            temp = atts.getValue("address");
            if (temp != null) currentFrame.address = temp;
            temp = atts.getValue("function");
            if (temp != null) currentFrame.function = temp;
            temp = atts.getValue("file");
            if (temp != null) currentFrame.file = temp;
            temp = atts.getValue("lineno");
            if (temp != null) currentFrame.lineno = temp;
            temp = atts.getValue("module");
            if (temp != null) currentFrame.module = temp;
        } else if (localName.equals("instruction")) {
            currentInstruction = new LInstruction();
            temp = atts.getValue("id");
            if (temp != null) currentInstruction.id = temp;
            temp = atts.getValue("address");
            if (temp != null) currentInstruction.address = temp;
            temp = atts.getValue("disassembly");
            if (temp != null) currentInstruction.disassembly = temp;
            temp = atts.getValue("function");
            if (temp != null) currentInstruction.function = temp;
            temp = atts.getValue("file");
            if (temp != null) currentInstruction.file = temp;
            temp = atts.getValue("lineno");
            if (temp != null) currentInstruction.lineno = temp;
            temp = atts.getValue("module");
            if (temp != null) currentInstruction.module = temp;
            temp = atts.getValue("text");
            if (temp != null) currentInstruction.text = temp;
        }
    }

    public void endElement(String namespaceURI, String localName, 
            String qualifiedName) throws SAXException {
        parseStatus.put(localName, Boolean.FALSE);
        if (localName.equals("message")) {
            if (currentMessage.type.equals("Cancellation") && currentMessage.priority.equals("999")) {
                currentMessage.priority = "ALL";
            }
            if (!currentMessage.time.equals("")) {
                try {
                    if (initialTime == 0)
                        initialTime = Integer.parseInt(currentMessage.time);
                    int time = Integer.parseInt(currentMessage.time) - initialTime;
                    time = time / 1000;
                    currentMessage.time = 
                        (new Integer(time)).toString();
                } catch (NumberFormatException e) {}
            }
            logfile.messages.add(currentMessage);
        } else if (localName.equals("trace")) {
            logfile.traces.put(currentTrace.id, currentTrace);
        } else if (localName.equals("instruction")) {
            logfile.instructions.put(currentInstruction.id, currentInstruction);
            logfile.instructionsByAddress.put(currentInstruction.address, currentInstruction);
        } else if (localName.equals("frame")) {
            currentTrace.frames.add(currentFrame);
        } else if (localName.equals("log")) {
            for (LMessage message : logfile.messages) {
                if (!message.backtraceID.equals("") && 
                        logfile.traces.containsKey(message.backtraceID)) {
                    message.backtrace = logfile.traces.get(message.backtraceID);
                }
                if (!message.instructionID.equals("") && 
                        logfile.instructions.containsKey(message.instructionID)) {
                    message.instruction = logfile.instructions.get(message.instructionID);
                }
            }
        }
    }

    public void characters(char[] ch, int start, int length) throws SAXException {
        int i;
        StringBuffer data = new StringBuffer();
        String str;
        for (i = start; i < start+length; i++) {
            data.append(ch[i]);
        }
        str = data.toString();
        if (parseStatus.get("app") == Boolean.TRUE) {
            logfile.appname += str;
        } else if (parseStatus.get("time") == Boolean.TRUE) {
            currentMessage.time += str;
        } else if (parseStatus.get("priority") == Boolean.TRUE) {
            currentMessage.priority += str;
        } else if (parseStatus.get("type") == Boolean.TRUE) {
            currentMessage.type += str;
        } else if (parseStatus.get("trace_id") == Boolean.TRUE) {
            currentMessage.backtraceID += str;
        } else if (parseStatus.get("inst_id") == Boolean.TRUE) {
            currentMessage.instructionID += str;
        } else if (parseStatus.get("label") == Boolean.TRUE) {
            currentMessage.label += str;
        } else if (parseStatus.get("details") == Boolean.TRUE) {
            currentMessage.details += str;
        } else if (parseStatus.get("id") == Boolean.TRUE &&
                   parseStatus.get("trace") == Boolean.TRUE) {
            currentTrace.id += str;
        } else if (parseStatus.get("id") == Boolean.TRUE &&
                   parseStatus.get("instruction") == Boolean.TRUE) {
            currentInstruction.id += str;
        } else if (parseStatus.get("disassembly") == Boolean.TRUE) {
            currentInstruction.disassembly += str;
        } else if (parseStatus.get("level") == Boolean.TRUE) {
            currentFrame.level += str;
        } else if (parseStatus.get("address") == Boolean.TRUE &&
                   parseStatus.get("frame") == Boolean.TRUE) {
            currentFrame.address += str;
        } else if (parseStatus.get("address") == Boolean.TRUE &&
                   parseStatus.get("instruction") == Boolean.TRUE) {
            currentInstruction.address += str;
        } else if (parseStatus.get("function") == Boolean.TRUE &&
                   parseStatus.get("frame") == Boolean.TRUE) {
            currentFrame.function += str;
        } else if (parseStatus.get("function") == Boolean.TRUE &&
                   parseStatus.get("instruction") == Boolean.TRUE) {
            currentInstruction.function += str;
        } else if (parseStatus.get("file") == Boolean.TRUE &&
                   parseStatus.get("frame") == Boolean.TRUE) {
            currentFrame.file += str;
        } else if (parseStatus.get("file") == Boolean.TRUE &&
                   parseStatus.get("instruction") == Boolean.TRUE) {
            currentInstruction.file += str;
        } else if (parseStatus.get("lineno") == Boolean.TRUE &&
                   parseStatus.get("frame") == Boolean.TRUE) {
            currentFrame.lineno += str;
        } else if (parseStatus.get("lineno") == Boolean.TRUE &&
                   parseStatus.get("instruction") == Boolean.TRUE) {
            currentInstruction.lineno += str;
        } else if (parseStatus.get("module") == Boolean.TRUE &&
                   parseStatus.get("frame") == Boolean.TRUE) {
            currentFrame.module += str;
        } else if (parseStatus.get("module") == Boolean.TRUE &&
                   parseStatus.get("instruction") == Boolean.TRUE) {
            currentInstruction.module += str;
        } else if (parseStatus.get("text") == Boolean.TRUE) {
            currentInstruction.text += str;
        }
    }

}

