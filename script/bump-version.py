#!/usr/bin/env python

import os
import re
import sys
import argparse

from lib.util import execute, get_electron_version, parse_version, scoped_cwd, \
is_nightly, is_beta, is_stable, get_next_nightly, get_next_beta, \
get_next_stable_from_pre, get_next_stable_from_stable, clean_parse_version

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def main():

  parser = argparse.ArgumentParser(
    description='Bump version numbers. Must specify at least one of the three'
               +' options:\n'
               +'   --bump=patch to increment patch version, or\n'
               +'   --stable to promote current beta to stable, or\n'
               +'   --version={version} to set version number directly\n'
               +'Note that you can use both --bump and --stable '
               +'simultaneously.',
               formatter_class=argparse.RawTextHelpFormatter
  )
  parser.add_argument(
    '--version',
    default=None,
    dest='new_version',
    help='new version number'
  )
  parser.add_argument(
    '--bump',
    action='store',
    default=None,
    dest='bump',
    help='increment [stable | beta | nightly]'
  )
  parser.add_argument(
    '--dry-run',
    action='store_true',
    default= False,
    dest='dry_run',
    help='just to check that version number is correct'
  )

  args = parser.parse_args()
  curr_version = get_electron_version()

  if args.bump not in ['stable', 'beta', 'nightly']:
    raise Exception('bump must be set to either stable, beta or nightly')

  if is_nightly(curr_version):
    if args.bump == 'nightly':
      version = get_next_nightly(curr_version)
    elif args.bump == 'beta':
      version = get_next_beta(curr_version)
    elif args.bump == 'stable':
      version = get_next_stable_from_pre(curr_version)
    else:
      not_reached()
  elif is_beta(curr_version):
    if args.bump == 'nightly':
      version = get_next_nightly(curr_version)
    elif args.bump == 'beta':
      version = get_next_beta(curr_version)
    elif args.bump == 'stable':
      version = get_next_stable_from_pre(curr_version)
    else:
      not_reached()
  elif is_stable(curr_version):
    if args.bump == 'nightly':
      version = get_next_nightly(curr_version)
    elif args.bump == 'beta':
      version = get_next_beta(curr_version)
    elif args.bump == 'stable':
      version = get_next_stable_from_stable(curr_version)
    else:
      not_reached()
  else:
    raise Exception("Invalid current version: " + curr_version)

  if args.new_version == None and args.bump == None and args.stable == False:
    parser.print_help()
    return 1

  versions = clean_parse_version(version)
  suffix = ''
  if '-' in version:
    suffix = '-' + version.split('-')[1]
    versions[3] = parse_version(version)[3]
  version = version.split('-')[0]

  if args.dry_run:
    print 'new version number would be: {0}\n'.format(version + suffix)
    return 0

  with scoped_cwd(SOURCE_ROOT):
    update_electron_gyp(version, suffix)
    update_win_rc(version, versions)
    update_version_h(versions, suffix)
    update_info_plist(version)
    update_package_json(version, suffix)
    tag_version(version, suffix)

  print 'Bumped to version: {0}'.format(version + suffix)

def not_reached():
  raise Exception('Unreachable code was reached')

def increase_version(versions, index):
  for i in range(index + 1, 4):
    versions[i] = '0'
  versions[index] = str(int(versions[index]) + 1)
  return versions


def update_electron_gyp(version, suffix):
  pattern = re.compile(" *'version%' *: *'[0-9.]+(-beta[0-9.]*)?(-dev)?"
    + "(-nightly[0-9.]*)?'")
  with open('electron.gyp', 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    if pattern.match(lines[i]):
      lines[i] = "    'version%': '{0}',\n".format(version + suffix)
      with open('electron.gyp', 'w') as f:
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


def update_version_h(versions, suffix):
  version_h = os.path.join('atom', 'common', 'atom_version.h')
  with open(version_h, 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    line = lines[i]
    if 'ATOM_MAJOR_VERSION' in line:
      lines[i] = '#define ATOM_MAJOR_VERSION {0}\n'.format(versions[0])
      lines[i + 1] = '#define ATOM_MINOR_VERSION {0}\n'.format(versions[1])
      lines[i + 2] = '#define ATOM_PATCH_VERSION {0}\n'.format(versions[2])

      if (suffix):
        lines[i + 3] = '#define ATOM_PRE_RELEASE_VERSION {0}\n'.format(suffix)
      else:
        lines[i + 3] = '// #define ATOM_PRE_RELEASE_VERSION\n'

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


def update_package_json(version, suffix):
  package_json = 'package.json'
  with open(package_json, 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    line = lines[i];
    if 'version' in line:
      lines[i] = '  "version": "{0}",\n'.format(version + suffix)
      break

  with open(package_json, 'w') as f:
    f.write(''.join(lines))


def tag_version(version, suffix):
  execute([
    'git',
    'commit',
    '-a',
    '-m',
    'Bump v{0}'.format(version + suffix),
    '-n'
  ])


if __name__ == '__main__':
  sys.exit(main())
