#!/usr/bin/env python3
import os
import subprocess
import sys

source = sys.argv[1]
dest = sys.argv[2]

# Ensure any existing framework is removed
subprocess.check_output(["rm", "-rf", dest])

subprocess.check_output(["cp", "-a", source, dest])

# Strip headers, we do not need to ship them
subprocess.check_output(["rm", "-r", os.path.join(dest, "Headers")])
subprocess.check_output(
    ["rm", "-r", os.path.join(dest, "Versions", "Current", "Headers")]
)
