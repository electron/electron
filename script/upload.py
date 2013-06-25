#!/usr/bin/env python

import errno
import glob
import os
import subprocess
import tempfile


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  try:
    ensure_s3put()
    upload()
  except AssertionError as e:
    return e.message


def ensure_s3put():
  output = ''
  try:
    output = subprocess.check_output(['s3put', '--help'])
  except OSError as e:
    if e.errno != errno.ENOENT:
      raise
  assert 'multipart' in output, 'Error: Please install boto and filechunkio'


def upload():
  os.chdir(SOURCE_ROOT)
  bucket, access_key, secret_key = s3_config()
  commit = subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()

  s3put(bucket, access_key, secret_key, 'atom-shell/{0}'.format(commit), glob.glob('atom-shell*.zip'))

  update_version(bucket, access_key, secret_key, commit)


def s3_config():
  config = (os.environ.get('ATOM_SHELL_S3_BUCKET', ''),
            os.environ.get('ATOM_SHELL_S3_ACCESS_KEY', ''),
            os.environ.get('ATOM_SHELL_S3_SECRET_KEY', ''))
  message = ('Error: Please set the $ATOM_SHELL_S3_BUCKET, '
             '$ATOM_SHELL_S3_ACCESS_KEY, and '
             '$ATOM_SHELL_S3_SECRET_KEY environment variables')
  assert all(len(c) for c in config), message
  return config


def update_version(bucket, access_key, secret_key, commit):
  version_path = os.path.join(SOURCE_ROOT, 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(commit)

  # Upload after file is closed, it's required under Windows.
  s3put(bucket, access_key, secret_key, 'atom-shell', [ version_path ])


def s3put(bucket, access_key, secret_key, key_prefix, files):
  args = [
    's3put',
    '--bucket', bucket,
    '--access_key', access_key,
    '--secret_key', secret_key,
    '--prefix', SOURCE_ROOT,
    '--key_prefix', key_prefix,
    '--grant', 'public-read'
  ] + files

  subprocess.check_call(args)


if __name__ == '__main__':
  import sys
  sys.exit(main())
