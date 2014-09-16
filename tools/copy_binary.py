#!/usr/bin/env python

import os
import shutil
import stat
import sys

src = sys.argv[1]
dist = sys.argv[2]

shutil.copyfile(src, dist)
os.chmod(dist, os.stat(dist).st_mode | stat.S_IEXEC)
