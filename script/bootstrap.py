#!/usr/bin/env python

import argparse
import errno
import os
import subprocess
import sys


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
BASE_URL = 'https://gh-contractor-zcbenz.s3.amazonaws.com/libchromiumcontent'


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  update_submodules()
  update_npm()
  bootstrap_brightray(args.url)
  update_atom_shell()


def parse_args():
  parser = argparse.ArgumentParser(description='Bootstrap this project')
  parser.add_argument('--url',
                      help='The base URL from which to download '
                      'libchromiumcontent (i.e., the URL you passed to '
                      'libchromiumcontent\'s script/upload script',
                      default=BASE_URL,
                      required=False)
  return parser.parse_args()


def update_submodules():
  subprocess.check_call(['git', 'submodule', 'sync', '--quiet'])
  subprocess.check_call(['git', 'submodule', 'update', '--init',
                         '--recursive'])


def update_npm():
  subprocess.check_call(['npm', 'install', 'npm', '--silent'])

  npm = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'npm')
  subprocess.check_call([npm, 'install', '--silent'])


def bootstrap_brightray(url):
  bootstrap = os.path.join(VENDOR_DIR, 'brightray', 'script', 'bootstrap')
  subprocess.check_call([sys.executable, bootstrap, url])


def update_atom_shell():
  update = os.path.join(SOURCE_ROOT, 'script', 'update.py')
  subprocess.check_call([sys.executable, update])


if __name__ == '__main__':
  sys.exit(main())
