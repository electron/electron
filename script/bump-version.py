#!/usr/bin/env python

import os
import re
import sys

from lib.util import execute, get_atom_shell_version, parse_version, scoped_cwd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  if len(sys.argv) != 2 or sys.argv[1] == '-h':
    print 'Usage: bump-version.py [<version> | major | minor | patch]'
    return 1

  option = sys.argv[1]
  increments = ['major', 'minor', 'patch', 'build']
  if option in increments:
    version = get_atom_shell_version()
    versions = parse_version(version.split('-')[0])
    versions = increase_version(versions, increments.index(option))
  else:
    versions = parse_version(option)

  version = '.'.join(versions[:3])

  with scoped_cwd(SOURCE_ROOT):
    update_atom_gyp(version)
    update_win_rc(version, versions)
    update_version_h(versions)
    update_info_plist(version)
    tag_version(version)


def increase_version(versions, index):
  for i in range(index + 1, 4):
    versions[i] = '0'
  versions[index] = str(int(versions[index]) + 1)
  return versions


def update_atom_gyp(version):
  pattern = re.compile(" *'version%' *: *'[0-9.]+'")
  with open('atom.gyp', 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    if pattern.match(lines[i]):
      lines[i] = "    'version%': '{0}',\n".format(version)
      with open('atom.gyp', 'w') as f:
        f.write(''.join(lines))
      return


def update_win_rc(version, versions):
  pattern_fv = re.compile(' FILEVERSION [0-9,]+')
  pattern_pv = re.compile(' PRODUCTVERSION [0-9,]+')
  pattern_fvs = re.compile(' *VALUE "FileVersion", "[0-9.]+"')
  pattern_pvs = re.compile(' *VALUE "ProductVersion", "[0-9.]+"')

  win_rc = os.path.join('atom', 'browser', 'resources', 'win', 'atom.rc')
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
  version_h = os.path.join('atom', 'common', 'atom_version.h')
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
  info_plist = os.path.join('atom', 'browser', 'resources', 'mac', 'Info.plist')
  with open(info_plist, 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    line = lines[i]
    if 'CFBundleVersion' in line:
      lines[i + 1] = '  <string>{0}</string>\n'.format(version)
    if 'CFBundleShortVersionString' in line:
      lines[i + 1] = '  <string>{0}</string>\n'.format(version)

  with open(info_plist, 'w') as f:
    f.write(''.join(lines))


def tag_version(version):
  execute(['git', 'commit', '-a', '-m', 'Bump v{0}'.format(version)])


if __name__ == '__main__':
  sys.exit(main())
