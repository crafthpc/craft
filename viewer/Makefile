FPCONF_CLASSES=ConfigEditorApp.class \
    ConfigFilter.class \
	ConfigTreeNode.class \
	ConfigTreeListener.class \
	ConfigTreeRenderer.class \
	OpenConfigAction.class \
	SaveConfigAction.class \
	BatchConfigAction.class \
	RemoveMovementAction.class \
	RemoveNonExecutedAction.class

FPVIEW_CLASSES=CancellationReport.class \
	ColumnResizer.class \
	InstructionComparator.class \
	InstructionHeaderRenderer.class \
	InstructionListener.class \
	InstructionModel.class \
	InstructionFuncReport.class \
	InstructionTypeReport.class \
	InstructionExecuteFuncReport.class \
	InstructionExecuteTypeReport.class \
	InstructionAssemblyReport.class \
	LFrame.class \
	LineNumberedBorder.class \
	LInstruction.class \
	LLogFile.class \
	LMessage.class \
	LogFileHandler.class \
	LogFilter.class \
	LShadowValue.class \
	LTrace.class \
	MatrixCellListener.class \
	MatrixModeListener.class \
	MergeLogAction.class \
	MessageComparator.class \
	MessageFilterListener.class \
	MessageFilterModel.class \
	MessageHeaderRenderer.class \
	MessageListener.class \
	MessageModel.class \
	OpenLogAction.class \
	ProgFilter.class \
	Report.class \
	ReportAction.class \
	ResetAction.class \
	RunProgramAction.class \
	ShadowValueComparator.class \
	ShadowValueModel.class \
	SummaryReport.class \
	TableHeaderListener.class \
	TabListener.class \
	TraceFrameListener.class \
	Util.class \
	VariableHeaderRenderer.class \
	VariableListener.class \
	ViewerApp.class

build: $(FPVIEW_CLASSES) $(FPCONF_CLASSES)

view: $(FPVIEW_CLASSES)
	java ViewerApp

conf: $(FPCONF_CLASSES)
	java ConfigEditorApp

%.class : %.java
	javac *.java

clean:
	rm -f $(FPVIEW_CLASSES) $(FPCONF_CLASSES)

.PHONY: build view conf clean

