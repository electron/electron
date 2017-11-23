#!/usr/bin/env python

import argparse
import errno
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

from io import StringIO
from lib.config import PLATFORM, get_target_arch,  get_env_var, s3_config, \
                       get_zip_name
from lib.util import electron_gyp, execute, get_electron_version, \
                     parse_version, scoped_cwd, s3put
from lib.github import GitHub


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

  if not dist_newer_than_head():
    run_python_script('create-dist.py')

  build_version = get_electron_build_version()
  if not ELECTRON_VERSION.startswith(build_version):
    error = 'Tag name ({0}) should match build version ({1})\n'.format(
        ELECTRON_VERSION, build_version)
    sys.stderr.write(error)
    sys.stderr.flush()
    return 1

  github = GitHub(auth_token())
  releases = github.repos(ELECTRON_REPO).releases.get()
  tag_exists = False
  for r in releases:
    if not r['draft'] and r['tag_name'] == args.version:
      release = r
      tag_exists = True
      break

  if not args.upload_to_s3:
    assert tag_exists == args.overwrite, \
          'You have to pass --overwrite to overwrite a published release'
    if not args.overwrite:
      release = create_or_get_release_draft(github, releases, args.version,
                                            tag_exists)

  # Upload Electron with GitHub Releases API.
  upload_electron(github, release, os.path.join(DIST_DIR, DIST_NAME),
                  args.upload_to_s3)
  if get_target_arch() != 'mips64el':
    upload_electron(github, release, os.path.join(DIST_DIR, SYMBOLS_NAME),
                    args.upload_to_s3)
  if PLATFORM == 'darwin':
    upload_electron(github, release, os.path.join(DIST_DIR,
                    'electron-api.json'), args.upload_to_s3)
    upload_electron(github, release, os.path.join(DIST_DIR, 'electron.d.ts'),
                    args.upload_to_s3)
    upload_electron(github, release, os.path.join(DIST_DIR, DSYM_NAME),
                    args.upload_to_s3)
  elif PLATFORM == 'win32':
    upload_electron(github, release, os.path.join(DIST_DIR, PDB_NAME),
                    args.upload_to_s3)

  # Upload free version of ffmpeg.
  ffmpeg = get_zip_name('ffmpeg', ELECTRON_VERSION)
  upload_electron(github, release, os.path.join(DIST_DIR, ffmpeg),
                  args.upload_to_s3)

  # Upload chromedriver and mksnapshot for minor version update.
  if parse_version(args.version)[2] == '0':
    chromedriver = get_zip_name('chromedriver', ELECTRON_VERSION)
    upload_electron(github, release, os.path.join(DIST_DIR, chromedriver),
                    args.upload_to_s3)
    mksnapshot = get_zip_name('mksnapshot', ELECTRON_VERSION)
    upload_electron(github, release, os.path.join(DIST_DIR, mksnapshot),
                    args.upload_to_s3)

  if PLATFORM == 'win32' and not tag_exists and not args.upload_to_s3:
    # Upload PDBs to Windows symbol server.
    run_python_script('upload-windows-pdb.py')

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


def get_text_with_editor(name):
  editor = os.environ.get('EDITOR', 'nano')
  initial_message = '\n# Please enter the body of your release note for %s.' \
                    % name

  t = tempfile.NamedTemporaryFile(suffix='.tmp', delete=False)
  t.write(initial_message)
  t.close()
  subprocess.call([editor, t.name])

  text = ''
  for line in open(t.name, 'r'):
    if len(line) == 0 or line[0] != '#':
      text += line

  os.unlink(t.name)
  return text

def create_or_get_release_draft(github, releases, tag, tag_exists):
  # Search for existing draft.
  for release in releases:
    if release['draft']:
      return release

  if tag_exists:
    tag = 'do-not-publish-me'
  return create_release_draft(github, tag)


def create_release_draft(github, tag):
  name = '{0} {1} beta'.format(PROJECT_NAME, tag)
  if os.environ.has_key('CI'):
    body = '(placeholder)'
  else:
    body = get_text_with_editor(name)
  if body == '':
    sys.stderr.write('Quit due to empty release note.\n')
    sys.exit(0)

  data = dict(tag_name=tag, name=name, body=body, draft=True, prerelease=True)
  r = github.repos(ELECTRON_REPO).releases.post(data=data)
  return r


def upload_electron(github, release, file_path, upload_to_s3):

  # if upload_to_s3 is set, skip github upload.
  if upload_to_s3:
    bucket, access_key, secret_key = s3_config()
    key_prefix = 'electron-artifacts/{0}'.format(release['tag_name'])
    s3put(bucket, access_key, secret_key, os.path.dirname(file_path),
          key_prefix, [file_path])
    upload_sha256_checksum(release['tag_name'], file_path, key_prefix)
    return

  # Delete the original file before uploading in CI.
  filename = os.path.basename(file_path)
  if os.environ.has_key('CI'):
    try:
      for asset in release['assets']:
        if asset['name'] == filename:
          github.repos(ELECTRON_REPO).releases.assets(asset['id']).delete()
    except Exception:
      pass

  # Upload the file.
  upload_io_to_github(release, filename, file_path)

  # Upload the checksum file.
  upload_sha256_checksum(release['tag_name'], file_path)

  # Upload ARM assets without the v7l suffix for backwards compatibility
  # TODO Remove for 2.0
  if 'armv7l' in filename:
    arm_filename = filename.replace('armv7l', 'arm')
    arm_file_path = os.path.join(os.path.dirname(file_path), arm_filename)
    shutil.copy2(file_path, arm_file_path)
    upload_electron(github, release, arm_file_path, upload_to_s3)


def upload_io_to_github(release, filename, filepath):
  print 'Uploading %s to Github' % \
      (filename)
  script_path = os.path.join(SOURCE_ROOT, 'script', 'upload-to-github.js')
  execute(['node', script_path, filepath, filename, str(release['id'])])


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


if __name__ == '__main__':
  import sys
  sys.exit(main())
