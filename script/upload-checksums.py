#!/usr/bin/env python

import argparse
import hashlib
import os
import tempfile

from lib.config import s3_config
from lib.util import download, rm_rf, s3put


DIST_URL = 'https://atom.io/download/atom-shell/'


def main():
  args = parse_args()

  url = DIST_URL + args.version + '/'
  directory, files = download_files(url, get_files_list(args.version))
  checksums = [
    create_checksum('sha1', directory, 'SHASUMS.txt', files),
    create_checksum('sha256', directory, 'SHASUMS256.txt', files)
  ]

  bucket, access_key, secret_key = s3_config()
  s3put(bucket, access_key, secret_key, directory,
        'atom-shell/dist/{0}'.format(args.version), checksums)

  rm_rf(directory)


def parse_args():
  parser = argparse.ArgumentParser(description='upload sumsha file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  return parser.parse_args()


def get_files_list(version):
  return [
    'node-{0}.tar.gz'.format(version),
    'iojs-{0}.tar.gz'.format(version),
    'iojs-{0}-headers.tar.gz'.format(version),
    'node.lib',
    'x64/node.lib',
    'win-x86/iojs.lib',
    'win-x64/iojs.lib',
  ]


def download_files(url, files):
  directory = tempfile.mkdtemp(prefix='electron-tmp')
  return directory, [
    download(f, url + f, os.path.join(directory, f))
    for f in files
  ]


def create_checksum(algorithm, directory, filename, files):
  lines = []
  for path in files:
    h = hashlib.new(algorithm)
    with open(path, 'r') as f:
      h.update(f.read())
      lines.append(h.hexdigest() + '  ' + os.path.relpath(path, directory))

  checksum_file = os.path.join(directory, filename)
  with open(checksum_file, 'w') as f:
    f.write('\n'.join(lines) + '\n')
  return checksum_file


if __name__ == '__main__':
  import sys
  sys.exit(main())
