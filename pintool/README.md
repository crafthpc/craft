## CRAFT: Configurable Runtime Analysis for Floating-point Tuning

Mike Lam, James Madison University

This folder contains Intel Pin versions of the CRAFT tools. These were developed
as an experiment and are not yet as fully-featured as the Dyninst-based
versions. There is also no support for in-place replacement or automated
precision searching.

To compile these tools, edit `build.sh` and set `PIN\_ROOT` appropriately then
run it. Note that you'll need to set this to the root of the full Pin
installation, not just the XED subfolder (as is needed by the original CRAFT
tools).

