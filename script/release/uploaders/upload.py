#!/usr/bin/env python

from __future__ import print_function
import argparse
import datetime
import errno
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.append(
  os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../.."))

from io import StringIO
from zipfile import ZipFile
from lib.config import PLATFORM, get_target_arch,  get_env_var, s3_config, \
                       get_zip_name
from lib.util import get_electron_branding, execute, get_electron_version, \
                     scoped_cwd, s3put, get_electron_exec, \
                     get_out_dir, SRC_DIR, ELECTRON_DIR


ELECTRON_REPO = 'electron/electron'
ELECTRON_VERSION = get_electron_version()

PROJECT_NAME = get_electron_branding()['project_name']
PRODUCT_NAME = get_electron_branding()['product_name']

OUT_DIR = get_out_dir()

DIST_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION)
SYMBOLS_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'symbols')
DSYM_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'dsym')
PDB_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'pdb')
DEBUG_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'debug')
TOOLCHAIN_PROFILE_NAME = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'toolchain-profile')


def main():
  args = parse_args()
  if  args.upload_to_s3:
    utcnow = datetime.datetime.utcnow()
    args.upload_timestamp = utcnow.strftime('%Y%m%d')

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
  # Rename dist.zip to  get_zip_name('electron', version, suffix='')
  electron_zip = os.path.join(OUT_DIR, DIST_NAME)
  shutil.copy2(os.path.join(OUT_DIR, 'dist.zip'), electron_zip)
  upload_electron(release, electron_zip, args)
  if get_target_arch() != 'mips64el':
    symbols_zip = os.path.join(OUT_DIR, SYMBOLS_NAME)
    shutil.copy2(os.path.join(OUT_DIR, 'symbols.zip'), symbols_zip)
    upload_electron(release, symbols_zip, args)
  if PLATFORM == 'darwin':
    api_path = os.path.join(ELECTRON_DIR, 'electron-api.json')
    upload_electron(release, api_path, args)

    ts_defs_path = os.path.join(ELECTRON_DIR, 'electron.d.ts')
    upload_electron(release, ts_defs_path, args)
    dsym_zip = os.path.join(OUT_DIR, DSYM_NAME)
    shutil.copy2(os.path.join(OUT_DIR, 'dsym.zip'), dsym_zip)
    upload_electron(release, dsym_zip, args)
  elif PLATFORM == 'win32':
    pdb_zip = os.path.join(OUT_DIR, PDB_NAME)
    shutil.copy2(os.path.join(OUT_DIR, 'pdb.zip'), pdb_zip)
    upload_electron(release, pdb_zip, args)
  elif PLATFORM == 'linux':
    debug_zip = os.path.join(OUT_DIR, DEBUG_NAME)
    shutil.copy2(os.path.join(OUT_DIR, 'debug.zip'), debug_zip)
    upload_electron(release, debug_zip, args)

  # Upload free version of ffmpeg.
  ffmpeg = get_zip_name('ffmpeg', ELECTRON_VERSION)
  ffmpeg_zip = os.path.join(OUT_DIR, ffmpeg)
  ffmpeg_build_path = os.path.join(SRC_DIR, 'out', 'ffmpeg', 'ffmpeg.zip')
  shutil.copy2(ffmpeg_build_path, ffmpeg_zip)
  upload_electron(release, ffmpeg_zip, args)

  chromedriver = get_zip_name('chromedriver', ELECTRON_VERSION)
  chromedriver_zip = os.path.join(OUT_DIR, chromedriver)
  shutil.copy2(os.path.join(OUT_DIR, 'chromedriver.zip'), chromedriver_zip)
  upload_electron(release, chromedriver_zip, args)

  mksnapshot = get_zip_name('mksnapshot', ELECTRON_VERSION)
  mksnapshot_zip = os.path.join(OUT_DIR, mksnapshot)
  if get_target_arch().startswith('arm'):
    # Upload the x64 binary for arm/arm64 mksnapshot
    mksnapshot = get_zip_name('mksnapshot', ELECTRON_VERSION, 'x64')
    mksnapshot_zip = os.path.join(OUT_DIR, mksnapshot)

  shutil.copy2(os.path.join(OUT_DIR, 'mksnapshot.zip'), mksnapshot_zip)
  upload_electron(release, mksnapshot_zip, args)

  if PLATFORM == 'linux' and get_target_arch() == 'x64':
    # Upload the hunspell dictionaries only from the linux x64 build
    hunspell_dictionaries_zip = os.path.join(OUT_DIR, 'hunspell_dictionaries.zip')
    upload_electron(release, hunspell_dictionaries_zip, args)

  if not tag_exists and not args.upload_to_s3:
    # Upload symbols to symbol server.
    run_python_upload_script('upload-symbols.py')
    if PLATFORM == 'win32':
      run_python_upload_script('upload-node-headers.py', '-v', args.version)

  if PLATFORM == 'win32':
    toolchain_profile_zip = os.path.join(OUT_DIR, TOOLCHAIN_PROFILE_NAME)
    with ZipFile(toolchain_profile_zip, 'w') as myzip:
      myzip.write(os.path.join(OUT_DIR, 'windows_toolchain_profile.json'), 'toolchain_profile.json')
    upload_electron(release, toolchain_profile_zip, args)


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


def run_python_upload_script(script, *args):
  script_path = os.path.join(
    ELECTRON_DIR, 'script', 'release', 'uploaders', script)
  return execute([sys.executable, script_path] + list(args))


def get_electron_build_version():
  if get_target_arch().startswith('arm') or os.environ.has_key('CI'):
    # In CI we just build as told.
    return ELECTRON_VERSION
  electron = get_electron_exec()
  return subprocess.check_output([electron, '--version']).strip()


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
    print('{0} uploaded to {1}/{2}/{0}'.format(filename, s3url, key_prefix))
    return

  # Upload the file.
  upload_io_to_github(release, filename, file_path, args.version)

  # Upload the checksum file.
  upload_sha256_checksum(args.version, file_path)


def upload_io_to_github(release, filename, filepath, version):
  print('Uploading %s to Github' % \
      (filename))
  script_path = os.path.join(
    ELECTRON_DIR, 'script', 'release', 'uploaders', 'upload-to-github.js')
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
  script_path = os.path.join(
    ELECTRON_DIR, 'script', 'release', 'find-github-release.js')
  release_info = execute(['node', script_path, version])
  release = json.loads(release_info)
  return release

if __name__ == '__main__':
  sys.exit(main())
