# CRAFT Viewers

To build with Ant, simply use the `ant` command in this directory.
If you don't have Ant installed, just use `make`.

This folder contains two viewers:

* `fpview` - Log viewer (useful for looking at fpinst and cancellation files
* `fpconfed` - Config viewer (useful for replacement analyses)

The "fpview" and "fpconfed" scripts in the /scripts directory should take care
of the details of starting these viewers. As long as the /scripts directory is
in your PATH, you should just be able to call them from your app directory:

    fpview fpinst.log               # to open the log viewer

    fpconfed craft_final.cfg        # to open the config viewer


