#!/usr/bin/env python

import sys
import os

from lib.util import safe_mkdir, extract_zip, tempdir, download


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
FRAMEWORKS_URL = 'https://gh-contractor-zcbenz.s3.amazonaws.com/frameworks'


def main():
  os.chdir(SOURCE_ROOT)
  safe_mkdir('frameworks')
  download_and_unzip('Quincy')
  download_and_unzip('Sparkle')


def download_and_unzip(framework):
  zip_path = download_framework(framework)
  if zip_path:
    extract_zip(zip_path, 'frameworks')


def download_framework(framework):
  framework_path = os.path.join('frameworks', framework) + '.framework'
  if os.path.exists(framework_path):
    return

  filename = framework + '.framework.zip'
  url = FRAMEWORKS_URL + '/' + filename
  download_dir = tempdir(prefix='atom-shell-')
  path = os.path.join(download_dir, filename)

  download('Download ' + framework, url, path)
  return path


if __name__ == '__main__':
  sys.exit(main())
