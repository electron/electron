#!/usr/bin/env python

import errno
import sys
import os

from lib.util import safe_mkdir, extract_zip, tempdir, download


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
FRAMEWORKS_URL = 'https://github.com/atom/atom-shell-frameworks/releases' \
                 '/download/v0.0.3'


def main():
  os.chdir(SOURCE_ROOT)
  try:
    os.makedirs('external_binaries')
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise
    else:
      return

  if sys.platform == 'darwin':
    download_and_unzip('Mantle')
    download_and_unzip('ReactiveCocoa')
    download_and_unzip('Squirrel')
  elif sys.platform in ['cygwin', 'win32']:
    download_and_unzip('directxsdk')


def download_and_unzip(framework):
  zip_path = download_framework(framework)
  if zip_path:
    extract_zip(zip_path, 'external_binaries')


def download_framework(framework):
  filename = framework + '.zip'
  url = FRAMEWORKS_URL + '/' + filename
  download_dir = tempdir(prefix='atom-shell-')
  path = os.path.join(download_dir, filename)

  download('Download ' + framework, url, path)
  return path


if __name__ == '__main__':
  sys.exit(main())
