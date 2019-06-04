#!/usr/bin/env python
import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))
BROWSERIFY = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'browserify')
if sys.platform == 'win32':
  BROWSERIFY += '.cmd'

deps = subprocess.check_output([BROWSERIFY, '--list'] + sys.argv[1:])
for dep in deps.split('\n'):
    if dep:
        dep = os.path.relpath(dep, SOURCE_ROOT)
        if sys.platform == 'win32':
            print('/'.join(dep.split('\\')))
        else:
            print(dep)
