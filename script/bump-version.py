#!/usr/bin/env python

import os
import re
import subprocess
import sys


def main():
  if len(sys.argv) != 2:
    print 'Usage: bump-version.py version'
    return 1

  version = sys.argv[1]
  if version[0] == 'v':
    version = version[1:]
  versions = parse_version(version)

  os.chdir(os.path.dirname(os.path.dirname(__file__)))
  update_package_json(version)


def parse_version(version):
  vs = version.split('.')
  if len(vs) > 4:
    return vs[0:4]
  else:
    return vs + [0] * (4 - len(vs))


def update_package_json(version):
  pattern = re.compile(' *"version" *: *"[0-9.]+"')
  with open('package.json', 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    if pattern.match(lines[i]):
      lines[i] = '  "version": "{0}",\n'.format(version)
      with open('package.json', 'w') as f:
        f.write(''.join(lines))
      return


if __name__ == '__main__':
  sys.exit(main())
