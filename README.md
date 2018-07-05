## CRAFT: Configurable Runtime Analysis for Floating-point Tuning

Mike Lam, James Madison University

Building and using this project currently requires some experience in systems
development and tool infrastructure. This file includes some basic installation
and suage instructions, but they may require some manual modification on your
specific platform. Contact the author if you encounter issues.


### Variable Mode (newer)


CRAFT now has support for a source-level search mode, enabled using the `-V`
command-line flag. In this mode, CRAFT performs type conversions in source code,
allowing it to search a variable space rather than an instruction space (as in
the binary mode described below).

Note that using variable mode means that you do not need to build the binary
instrumentation part of CRAFT, which negates the instructions in the section
below; all you need is to make sure the `scripts` folder is in your `PATH` and
that you have Ruby 2.0 or later installed.

To use this mode, you should first clean and build your project using
`craft_find_variables` instead of `CC` or `CXX`. This should produce a file
named `craft_orig.cfg` that contains the base list of variables in your project
that can be tuned.

The next step is to perform a search to find a mixed-precision version of your
program with speedup, subject to a verification procedure that you provide.

To begin, create a new folder and copy the `craft_orig.cfg` into it. You will
also need to write a new script called `craft_driver`
([sample](scripts/craft_driver_source)), which will be run in a new, empty
folder for every candidate configuration generated during the search process.
This script is similar to the identically-named script used in the binary mode
except it should also handle copying your original project files to the current
directory, and it should build your project using the following instead of `CC`
or `CXX` (the `$1` is a parameter that will be provided by CRAFT):

    craft_replace_types $1

In addition, the `craft_driver` script should handle running your project with a
representative input, and it should write results to standard out using the
following format:

    status:  [pass|fail|error]
    time:    <seconds>
    error:   <error>

The status must be either `pass`, `fail`, or `error` and the time must be in
seconds (and may be fractional). The error is optional but if present should be
given as a scalar double-precision number. Unlike the binary mode, you should
run your program using your regular executable name.

Finally, run the search using the following command from the search folder:

    craft search -V

If you have multiple cores, it is recommended that you use the `-j <N>` option
to run up to N multiple configurations in parallel.

The search will print various status information while it is running. When it is
finished, you will find your final recommended configuration in the `final`
subfolder. You may examine `craft_final.cfg` for a list of converted variables.

This support is still very brittle and experimental, and it is under active
development as of Summer 2018. More documentation is on the way. Please contact
the author if you have questions or run into issues.

Eventually these source file configurations will be generated using the
[TypeForge tool](https://github.com/rose-compiler/rose-develop/blob/master/projects/CodeThorn/src/typeforge.C)
created using the [Rose compiler framework](https://github.com/rose-compiler/rose-develop),
but for now there are some temporary work-arounds provided in this repository.


### Binary Mode (original)

Currently this project only works on x86\_64 Linux. The environment variable
`PLATFORM` needs to be set to "`x86_64-unknown-linux2.4`".

I have provided a rudimentary Bash build/installation script (`build.sh`) that
will acquire dependencies and build everything; it should work as long as the
system Boost headers are compatible (this is to save space in the common case).
This script will also create a Bash environment script (`env-setup.sh`) that you
should source every time you want to run CRAFT (or you could put it in your Bash
profile).
If you need to set up a build environment manually, here is a list of the
dependencies:

* DyninstAPI (Tested version: [9.1.0](https://github.com/dyninst/dyninst))

  Dyninst must be compiled and accessible to the linker. The environment variable
  `DYNINST_ROOT` should point to the location it is installed.

* XED2 Library (Tested version: from
  [Pin 2.14-71313](http://software.intel.com/en-us/articles/pintool-downloads))

  Set the `XED_KIT` variable in the Makefile appropriately after you have
  downloaded Pin. There is no need to compile Pin or XED from source. Note that
  CRAFT does not use Pin directly, only XED.

* Boost headers (Tested version:
  [1.53.0](http://sourceforge.net/projects/boost/files/boost/1.53.0/))

  The default Boost headers on your system may work. If you get Boost-related
  compile errors, download the headers from the URL above and use the
  `LOCAL_INC_DIRS` variable in the Makefile to point the compiler to it (using
  `-I`).

You will also need a reasonably recent (4.x) version of gcc. The build has been
tested with gcc version 4.6.2 and 4.9.3.

To build CRAFT manually once the dependencies are in place, use the `make`
command in the root directory. You will also need to add the `$PLATFORM`
folder created by the build process to `PATH` and `LD_LIBRARY_PATH`. I recommend
writing a shell script to set `PLATFORM`, `DYNINST_ROOT`, `PATH`, and
`LD_LIBRARY_PATH`.

For replacement analysis to work, the current directory ("`.`") also needs to be
in `LD_LIBRARY_PATH` so that the runtime linker can find the rewritten shared
libraries.

To test the build, execute the `./run_tests` command in the testsuite folder.

To sanity-check a non-testsuite executable, try the following commands:

    fpinst --cinst <binary>
    fpinst --dcancel <binary>
    fpinst --svinp double <binary>

To use the automatic search script (`craft` in the `scripts` folder), you will
need Ruby 2.0 or later.

To build and use the GUI tools in the `viewer` folder, you will need Java 1.6 or
later.

For further instructions and a short tutorial, see the
[Sum2PI_X](demo/sum2pi_x/README) example/demo.


### License Notice

CRAFT is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.

For more license details, please read LICENSE in the main project directory.

