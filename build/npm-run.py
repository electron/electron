#!/usr/bin/env python
import os
import sys

SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))
args = ["npm", "run",
    "--prefix",
    SOURCE_ROOT
    ] + sys.argv[1:]
os.execvp("npm", args)
