#!/usr/bin/env python

from __future__ import print_function
import sys

from lib.util import get_electron_version

def main():
  print(get_electron_version())

if __name__ == '__main__':
  sys.exit(main())
