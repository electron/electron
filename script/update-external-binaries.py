#!/usr/bin/env python

import argparse
import errno
import sys
import os

from lib.config import PLATFORM, get_target_arch
from lib.util import add_exec_bit, download, extract_zip, rm_rf, \
                     safe_mkdir, tempdir

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def parse_args():
  parser = argparse.ArgumentParser(
      description='Download binaries for Electron build')

  parser.add_argument('-u', '--root-url', required=True,
                      help="Root URL for all downloads.")
  parser.add_argument('-v', '--version', required=True,
                      help="Version string, e.g. 'v1.0.0'.")

  return parser.parse_args()

def main():
  args = parse_args()
  url_prefix = "{root_url}/{version}".format(**vars(args))

  os.chdir(SOURCE_ROOT)
  version_file = os.path.join(SOURCE_ROOT, 'external_binaries', '.version')

  if (is_updated(version_file, args.version)):
    return

  rm_rf('external_binaries')
  safe_mkdir('external_binaries')

  if sys.platform == 'darwin':
    download_and_unzip(url_prefix, 'Mantle')
    download_and_unzip(url_prefix, 'ReactiveCocoa')
    download_and_unzip(url_prefix, 'Squirrel')
  elif sys.platform in ['cygwin', 'win32']:
    download_and_unzip(url_prefix, 'directxsdk-' + get_target_arch())

  # get sccache & set exec bit. https://bugs.python.org/issue15795
  download_and_unzip(url_prefix, 'sccache-{0}-x64'.format(PLATFORM))
  appname = 'sccache'
  if sys.platform == 'win32':
    appname += '.exe'
  add_exec_bit(os.path.join('external_binaries', appname))

  with open(version_file, 'w') as f:
    f.write(args.version)


def is_updated(version_file, version):
  existing_version = ''
  try:
    with open(version_file, 'r') as f:
      existing_version = f.readline().strip()
  except IOError as e:
    if e.errno != errno.ENOENT:
      raise
  return existing_version == version


def download_and_unzip(url_prefix, framework):
  zip_path = download_framework(url_prefix, framework)
  if zip_path:
    extract_zip(zip_path, 'external_binaries')


def download_framework(url_prefix, framework):
  filename = framework + '.zip'
  url = url_prefix + '/' + filename
  download_dir = tempdir(prefix='electron-')
  path = os.path.join(download_dir, filename)

  download('Download ' + framework, url, path)
  return path


if __name__ == '__main__':
  sys.exit(main())
