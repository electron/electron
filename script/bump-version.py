#!/usr/bin/env python

import os
import re
import subprocess
import sys


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  if len(sys.argv) != 2:
    print 'Usage: bump-version.py version'
    return 1

  version = sys.argv[1]
  if version[0] == 'v':
    version = version[1:]
  versions = parse_version(version)

  os.chdir(SOURCE_ROOT)
  update_package_json(version)
  update_win_rc(version, versions)
  update_version_h(versions)
  update_info_plist(version)
  tag_version(version)


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


def update_win_rc(version, versions):
  pattern_fv = re.compile(' FILEVERSION [0-9,]+')
  pattern_pv = re.compile(' PRODUCTVERSION [0-9,]+')
  pattern_fvs = re.compile(' *VALUE "FileVersion", "[0-9.]+"')
  pattern_pvs = re.compile(' *VALUE "ProductVersion", "[0-9.]+"')

  win_rc = os.path.join('app', 'win', 'atom.rc')
  with open(win_rc, 'r') as f:
    lines = f.readlines()

  versions = [str(v) for v in versions]
  for i in range(0, len(lines)):
    line = lines[i]
    if pattern_fv.match(line):
      lines[i] = ' FILEVERSION {0}\r\n'.format(','.join(versions))
    elif pattern_pv.match(line):
      lines[i] = ' PRODUCTVERSION {0}\r\n'.format(','.join(versions))
    elif pattern_fvs.match(line):
      lines[i] = '            VALUE "FileVersion", "{0}"\r\n'.format(version)
    elif pattern_pvs.match(line):
      lines[i] = '            VALUE "ProductVersion", "{0}"\r\n'.format(version)

  with open(win_rc, 'w') as f:
    f.write(''.join(lines))


def update_version_h(versions):
  version_h = os.path.join('common', 'atom_version.h')
  with open(version_h, 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    line = lines[i]
    if 'ATOM_MAJOR_VERSION' in line:
      lines[i] = '#define ATOM_MAJOR_VERSION {0}\n'.format(versions[0])
      lines[i + 1] = '#define ATOM_MINOR_VERSION {0}\n'.format(versions[1])
      lines[i + 2] = '#define ATOM_PATCH_VERSION {0}\n'.format(versions[2])

      with open(version_h, 'w') as f:
        f.write(''.join(lines))
      return


def update_info_plist(version):
  info_plist = os.path.join('browser', 'mac', 'Info.plist')
  with open(info_plist, 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    line = lines[i]
    if 'CFBundleVersion' in line:
      lines[i + 1] = '  <string>{0}</string>\n'.format(version)

      with open(info_plist, 'w') as f:
        f.write(''.join(lines))
      return


def tag_version(version):
  subprocess.check_call(['git', 'commit', '-a', '-m',
                         'Bump v{0}.'.format(version)])
  subprocess.check_call(['git', 'tag', 'v{0}'.format(version)])


if __name__ == '__main__':
  sys.exit(main())
