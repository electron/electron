#!/usr/bin/env python

import argparse
import os
import sys

from lib.util import rm_rf


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()

  remove_directory('dist')
  remove_directory('out')

  if not args.build:
    remove_directory('node_modules')
    remove_directory('spec/node_modules')

    remove_directory('vendor/download/libchromiumcontent')
    remove_directory('vendor/libchromiumcontent/src')

    remove_directory(os.path.expanduser('~/.node-gyp'))


def parse_args():
  parser = argparse.ArgumentParser(description='Remove generated and' \
                                               'downloaded build files')
  parser.add_argument('-b', '--build',
                      help='Only remove out and dist directories',
                      action='store_true')
  return parser.parse_args()


def remove_directory(directory):
  print 'Removing %s' % directory
  rm_rf(directory)


if __name__ == '__main__':
  sys.exit(main())
