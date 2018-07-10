#!/usr/bin/env python

# Download individual checksum files for Electron zip files from S3,
# concatenate them, and upload to GitHub.

from __future__ import print_function

import argparse
import sys

from lib.config import s3_config
from lib.util import boto_path_dirs

sys.path.extend(boto_path_dirs())
from boto.s3.connection import S3Connection


def main():
  args = parse_args()

  bucket_name, access_key, secret_key = s3_config()
  s3 = S3Connection(access_key, secret_key)
  bucket = s3.get_bucket(bucket_name)
  if bucket is None:
    print('S3 bucket "{}" does not exist!'.format(bucket_name), file=sys.stderr)
    return 1

  prefix = 'atom-shell/tmp/{0}/'.format(args.version)
  shasums = [s3_object.get_contents_as_string().strip()
             for s3_object in bucket.list(prefix, delimiter='/')
             if s3_object.key.endswith('.sha256sum')]
  print('\n'.join(shasums))
  return 0


def parse_args():
  parser = argparse.ArgumentParser(description='Upload SHASUMS files to GitHub')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  return parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
