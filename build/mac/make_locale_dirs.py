# usage: make_locale_dirs.py locale_dir [...]
#
# This script is intended to create empty locale directories (.lproj) in a
# Cocoa .app bundle. The presence of these empty directories is sufficient to
# convince Cocoa that the application supports the named localization, even if
# an InfoPlist.strings file is not provided. Chrome uses these empty locale
# directoires for its helper executable bundles, which do not otherwise
# require any direct Cocoa locale support.

import os
import sys


def main(args):
  for dirname in args:
    os.makedirs(dirname)


if __name__ == '__main__':
  main(sys.argv[1:])
