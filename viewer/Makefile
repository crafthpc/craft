# Makefile for CRAFT viewer
#
# Included in case the user doesn't have Ant installed.
# The Ant project is the preferred method of building this project.
#

build:
	mkdir -p classes
	javac -d classes src/*.java
	echo "Main-Class: ViewerApp" >fpview.mf
	echo "Main-Class: ConfigEditorApp" >fpconfed.mf
	cd classes && jar cfm ../fpview.jar ../fpview.mf *.class
	cd classes && jar cfm ../fpconfed.jar ../fpconfed.mf *.class
	rm -f fpview.mf fpconfed.mf

clean:
	@rm -rf classes
	@rm -f fpview.jar fpconfed.jar

.PHONY: build clean

