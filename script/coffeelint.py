#!/usr/bin/env python

import errno
import glob
import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  os.chdir(SOURCE_ROOT)

  coffeelint = os.path.join(SOURCE_ROOT, 'node_modules', 'coffeelint', 'bin',
                            'coffeelint')
  settings = ['--quiet', '-f', os.path.join('script', 'coffeelint.json')]
  files = glob.glob('browser/api/lib/*.coffee') + \
          glob.glob('renderer/api/lib/*.coffee') + \
          glob.glob('common/api/lib/*.coffee') + \
          glob.glob('browser/atom/*.coffee')

  try:
    subprocess.check_call(['node', coffeelint] + settings + files)
  except OSError as e:
    if e.errno == errno.ENOENT and sys.platform in ['win32', 'cygwin']:
      subprocess.check_call(['node', coffeelint] + settings + files,
                            executable='C:/Program Files/nodejs/node.exe')
    else:
      raise


if __name__ == '__main__':
  sys.exit(main())
