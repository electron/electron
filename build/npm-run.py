#!/usr/bin/env python3

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
    error_msg = "NPM script '{}' failed with code '{}':\n".format(sys.argv[2], e.returncode)
    print(error_msg + e.output.decode('utf8'))
    sys.exit(e.returncode)
