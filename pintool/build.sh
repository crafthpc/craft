#!/bin/bash

# should point to Pin installation
PIN_ROOT="$HOME/opt/pin-3.6-97554-g31f0a167d-gcc-linux"
#PIN_ROOT="$HOME/opt/pin-2.14-71313-gcc.4.4.7-linux"

# create output folder
mkdir -p obj-intel64

# debug mode
#PIN_ROOT=$PIN_ROOT DEBUG=1 make -j12 $@

# release mode
PIN_ROOT=$PIN_ROOT make clean
PIN_ROOT=$PIN_ROOT make -j12 $@

