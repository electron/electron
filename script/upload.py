#!/usr/bin/env python

import argparse
import datetime
import errno
import hashlib
import json
import os
import subprocess
import sys
import tempfile

from io import StringIO
from lib.config import PLATFORM, get_target_arch,  get_env_var, s3_config, \
                       get_zip_name
from lib.util import electron_gyp, execute, get_electron_version, \
                     parse_version, scoped_cwd, s3put


ELECTRON_REPO = 'electron/electron'
ELECTRON_VERSION = get_electron_version()

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')

DIST_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION)
SYMBOLS_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'symbols')
DSYM_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'dsym')
PDB_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'pdb')


def main():
  args = parse_args()
  if  args.upload_to_s3:
    utcnow = datetime.datetime.utcnow()
    args.upload_timestamp = utcnow.strftime('%Y%m%d')

  if not dist_newer_than_head():
    run_python_script('create-dist.py')

  build_version = get_electron_build_version()
  if not ELECTRON_VERSION.startswith(build_version):
    error = 'Tag name ({0}) should match build version ({1})\n'.format(
        ELECTRON_VERSION, build_version)
    sys.stderr.write(error)
    sys.stderr.flush()
    return 1

  tag_exists = False
  release = get_release(args.version)
  if not release['draft']:
    tag_exists = True

  if not args.upload_to_s3:
    assert release['exists'], 'Release does not exist; cannot upload to GitHub!'
    assert tag_exists == args.overwrite, \
          'You have to pass --overwrite to overwrite a published release'

  # Upload Electron files.
  upload_electron(release, os.path.join(DIST_DIR, DIST_NAME), args)
  if get_target_arch() != 'mips64el':
    upload_electron(release, os.path.join(DIST_DIR, SYMBOLS_NAME), args)
  if PLATFORM == 'darwin':
    upload_electron(release, os.path.join(DIST_DIR, 'electron-api.json'), args)
    upload_electron(release, os.path.join(DIST_DIR, 'electron.d.ts'), args)
    upload_electron(release, os.path.join(DIST_DIR, DSYM_NAME), args)
  elif PLATFORM == 'win32':
    upload_electron(release, os.path.join(DIST_DIR, PDB_NAME), args)

  # Upload free version of ffmpeg.
  ffmpeg = get_zip_name('ffmpeg', ELECTRON_VERSION)
  upload_electron(release, os.path.join(DIST_DIR, ffmpeg), args)

  chromedriver = get_zip_name('chromedriver', ELECTRON_VERSION)
  upload_electron(release, os.path.join(DIST_DIR, chromedriver), args)
  mksnapshot = get_zip_name('mksnapshot', ELECTRON_VERSION)
  upload_electron(release, os.path.join(DIST_DIR, mksnapshot), args)

  if get_target_arch().startswith('arm'):
    # Upload the x64 binary for arm/arm64 mksnapshot
    mksnapshot = get_zip_name('mksnapshot', ELECTRON_VERSION, 'x64')
    upload_electron(release, os.path.join(DIST_DIR, mksnapshot), args)

  if not tag_exists and not args.upload_to_s3:
    # Upload symbols to symbol server.
    run_python_script('upload-symbols.py')
    if PLATFORM == 'win32':
      # Upload node headers.
      run_python_script('create-node-headers.py', '-v', args.version)
      run_python_script('upload-node-headers.py', '-v', args.version)


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      default=ELECTRON_VERSION)
  parser.add_argument('-o', '--overwrite',
                      help='Overwrite a published release',
                      action='store_true')
  parser.add_argument('-p', '--publish-release',
                      help='Publish the release',
                      action='store_true')
  parser.add_argument('-s', '--upload_to_s3',
                      help='Upload assets to s3 bucket',
                      dest='upload_to_s3',
                      action='store_true',
                      default=False,
                      required=False)
  return parser.parse_args()


def run_python_script(script, *args):
  script_path = os.path.join(SOURCE_ROOT, 'script', script)
  return execute([sys.executable, script_path] + list(args))


def get_electron_build_version():
  if get_target_arch().startswith('arm') or os.environ.has_key('CI'):
    # In CI we just build as told.
    return ELECTRON_VERSION
  if PLATFORM == 'darwin':
    electron = os.path.join(SOURCE_ROOT, 'out', 'R',
                              '{0}.app'.format(PRODUCT_NAME), 'Contents',
                              'MacOS', PRODUCT_NAME)
  elif PLATFORM == 'win32':
    electron = os.path.join(SOURCE_ROOT, 'out', 'R',
                              '{0}.exe'.format(PROJECT_NAME))
  else:
    electron = os.path.join(SOURCE_ROOT, 'out', 'R', PROJECT_NAME)

  return subprocess.check_output([electron, '--version']).strip()


def dist_newer_than_head():
  with scoped_cwd(SOURCE_ROOT):
    try:
      head_time = subprocess.check_output(['git', 'log', '--pretty=format:%at',
                                           '-n', '1']).strip()
      dist_time = os.path.getmtime(os.path.join(DIST_DIR, DIST_NAME))
    except OSError as e:
      if e.errno != errno.ENOENT:
        raise
      return False

  return dist_time > int(head_time)


def upload_electron(release, file_path, args):
  filename = os.path.basename(file_path)

  # if upload_to_s3 is set, skip github upload.
  if args.upload_to_s3:
    bucket, access_key, secret_key = s3_config()
    key_prefix = 'electron-artifacts/{0}_{1}'.format(args.version,
                                                     args.upload_timestamp)
    s3put(bucket, access_key, secret_key, os.path.dirname(file_path),
          key_prefix, [file_path])
    upload_sha256_checksum(args.version, file_path, key_prefix)
    s3url = 'https://gh-contractor-zcbenz.s3.amazonaws.com'
    print '{0} uploaded to {1}/{2}/{0}'.format(filename, s3url, key_prefix)
    return

  # Upload the file.
  upload_io_to_github(release, filename, file_path, args.version)

  # Upload the checksum file.
  upload_sha256_checksum(args.version, file_path)


def upload_io_to_github(release, filename, filepath, version):
  print 'Uploading %s to Github' % \
      (filename)
  script_path = os.path.join(SOURCE_ROOT, 'script', 'upload-to-github.js')
  execute(['node', script_path, filepath, filename, str(release['id']),
          version])


def upload_sha256_checksum(version, file_path, key_prefix=None):
  bucket, access_key, secret_key = s3_config()
  checksum_path = '{}.sha256sum'.format(file_path)
  if key_prefix is None:
    key_prefix = 'atom-shell/tmp/{0}'.format(version)
  sha256 = hashlib.sha256()
  with open(file_path, 'rb') as f:
    sha256.update(f.read())

  filename = os.path.basename(file_path)
  with open(checksum_path, 'w') as checksum:
    checksum.write('{} *{}'.format(sha256.hexdigest(), filename))
  s3put(bucket, access_key, secret_key, os.path.dirname(checksum_path),
        key_prefix, [checksum_path])


def auth_token():
  token = get_env_var('GITHUB_TOKEN')
  message = ('Error: Please set the $ELECTRON_GITHUB_TOKEN '
             'environment variable, which is your personal token')
  assert token, message
  return token


def get_release(version):
  script_path = os.path.join(SOURCE_ROOT, 'script', 'find-release.js')
  release_info = execute(['node', script_path, version])
  release = json.loads(release_info)
  return release

if __name__ == '__main__':
  import sys
  sys.exit(main())
