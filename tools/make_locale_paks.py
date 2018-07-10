#!/usr/bin/env python

# usage: make_locale_paks build_dir [...]
#
# This script creates the .pak files under locales directory, it is used to fool
# the ResourcesBundle that the locale is available.

import errno
import sys
import os


def main():
  target_dir = sys.argv[1]
  locale_dir = os.path.join(target_dir, 'locales')
  safe_mkdir(locale_dir)
  for pak in sys.argv[2:]:
    touch(os.path.join(locale_dir, pak + '.pak'))


def touch(filename):
  with open(filename, 'w+'):
    pass


def safe_mkdir(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise


if __name__ == '__main__':
  sys.exit(main())
