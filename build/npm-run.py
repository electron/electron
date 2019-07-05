#!/usr/bin/env python
from __future__ import print_function
import os
import subprocess
import sys

SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))
cmd = "npm"
if sys.platform == "win32":
    cmd += ".cmd"
args = [cmd, "run",
    "--prefix",
    SOURCE_ROOT
    ] + sys.argv[1:]
try:
    subprocess.check_output(args, stderr=subprocess.STDOUT)
except subprocess.CalledProcessError as e:
    print("NPM script '" + sys.argv[2] + "' failed with code '" + str(e.returncode) + "':\n" + e.output)
    sys.exit(e.returncode)
