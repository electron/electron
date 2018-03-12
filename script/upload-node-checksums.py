#!/usr/bin/env python

import argparse
import errno
import hashlib
import os
import shutil
import tempfile

from lib.config import s3_config
from lib.util import download, rm_rf, s3put


DIST_URL = 'https://atom.io/download/electron/'


def main():
  args = parse_args()
  dist_url = args.dist_url
  if dist_url[len(dist_url) - 1] != "/":
    dist_url += "/"

  url = dist_url + args.version + '/'
  directory, files = download_files(url, get_files_list(args.version))
  checksums = [
    create_checksum('sha1', directory, 'SHASUMS.txt', files),
    create_checksum('sha256', directory, 'SHASUMS256.txt', files)
  ]

  if args.target_dir is None:
    bucket, access_key, secret_key = s3_config()
    s3put(bucket, access_key, secret_key, directory,
          'atom-shell/dist/{0}'.format(args.version), checksums)
  else:
    copy_files(checksums, args.target_dir)

  rm_rf(directory)


def parse_args():
  parser = argparse.ArgumentParser(description='upload sumsha file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  parser.add_argument('-u', '--dist-url', help='Specify the dist url for downloading',
                      required=False, default=DIST_URL)
  parser.add_argument('-t', '--target-dir', help='Specify target dir of checksums',
                      required=False)
  return parser.parse_args()


def get_files_list(version):
  return [
    ['node-{0}.tar.gz'.format(version), True],
    ['iojs-{0}.tar.gz'.format(version), True],
    ['iojs-{0}-headers.tar.gz'.format(version), True],
    ['node.lib', False],
    ['x64/node.lib', False],
    ['win-x86/iojs.lib', False],
    ['win-x64/iojs.lib', False]
  ]


def download_files(url, files):
  directory = tempfile.mkdtemp(prefix='electron-tmp')
  result = []
  for f in files:
    required = f[1]
    try:
      result.append(download(f[0], url + f[0], os.path.join(directory, f[0])))
    except:
      if required:
        raise

  return directory, result


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

def copy_files(source_files, output_dir):
  for source_file in source_files:
    output_path = os.path.join(output_dir, os.path.basename(source_file))
    safe_mkdir(os.path.dirname(output_path))
    shutil.copy2(source_file, output_path)

def safe_mkdir(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise


if __name__ == '__main__':
  import sys
  sys.exit(main())
