#!/usr/bin/env bash
#
# Debug script: tests json2cfg, cfg2json, and merge_cfgs.rb (from FloatSmith)
# scripts by running them repeatedly and diffing the results. Should be run from
# a folder with a completed variable search. The output is self-explanatory.
#
# Requires that both CRAFT and FloatSmith be installed and that both of their
# scripts folders be in your PATH.
#

json2cfg craft_orig.json passed/* >tmp1.cfg
cfg2json tmp1.cfg >tmp1.json
json2cfg craft_orig.json tmp1.json >tmp2.cfg
cfg2json tmp2.cfg >tmp2.json

merge_cfgs.rb passed/* >tmp3.json

echo
echo "== Diff #1 (cfg files) - should be blank:"
diff tmp1.cfg tmp2.cfg
echo
echo "== Diff #2 (json files) - should be blank:"
diff tmp1.json tmp2.json
echo
echo "== Diff #3 (json files) - should only be tool ID (CRAFT vs. FloatSmith)"
diff tmp1.json tmp3.json
echo
