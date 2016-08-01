#!/usr/bin/env python

import argparse
import errno
from io import StringIO
import os
import subprocess
import sys
import tempfile

from lib.config import PLATFORM, get_target_arch, get_chromedriver_version, \
                       get_platform_key, get_env_var
from lib.util import electron_gyp, execute, get_electron_version, \
                     parse_version, scoped_cwd
from lib.github import GitHub


ELECTRON_REPO = 'electron/electron'
ELECTRON_VERSION = get_electron_version()

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
DIST_NAME = '{0}-{1}-{2}-{3}.zip'.format(PROJECT_NAME,
                                         ELECTRON_VERSION,
                                         get_platform_key(),
                                         get_target_arch())
SYMBOLS_NAME = '{0}-{1}-{2}-{3}-symbols.zip'.format(PROJECT_NAME,
                                                    ELECTRON_VERSION,
                                                    get_platform_key(),
                                                    get_target_arch())
DSYM_NAME = '{0}-{1}-{2}-{3}-dsym.zip'.format(PROJECT_NAME,
                                              ELECTRON_VERSION,
                                              get_platform_key(),
                                              get_target_arch())
PDB_NAME = '{0}-{1}-{2}-{3}-pdb.zip'.format(PROJECT_NAME,
                                            ELECTRON_VERSION,
                                            get_platform_key(),
                                            get_target_arch())


def main():
  args = parse_args()

  if not args.publish_release:
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
  for release in releases:
    if not release['draft'] and release['tag_name'] == args.version:
      tag_exists = True
      break

  release = create_or_get_release_draft(github, releases, args.version,
                                        tag_exists)

  if args.publish_release:
    # Upload the Node SHASUMS*.txt.
    run_python_script('upload-node-checksums.py', '-v', ELECTRON_VERSION)

    # Upload the index.json.
    run_python_script('upload-index-json.py')

    # Create and upload the Electron SHASUMS*.txt
    release_electron_checksums(github, release)

    # Press the publish button.
    publish_release(github, release['id'])

    # Do not upload other files when passed "-p".
    return

  # Upload Electron with GitHub Releases API.
  upload_electron(github, release, os.path.join(DIST_DIR, DIST_NAME))
  upload_electron(github, release, os.path.join(DIST_DIR, SYMBOLS_NAME))
  if PLATFORM == 'darwin':
    upload_electron(github, release, os.path.join(DIST_DIR, DSYM_NAME))
  elif PLATFORM == 'win32':
    upload_electron(github, release, os.path.join(DIST_DIR, PDB_NAME))

  # Upload free version of ffmpeg.
  ffmpeg = 'ffmpeg-{0}-{1}-{2}.zip'.format(
      ELECTRON_VERSION, get_platform_key(), get_target_arch())
  upload_electron(github, release, os.path.join(DIST_DIR, ffmpeg))

  # Upload chromedriver and mksnapshot for minor version update.
  if parse_version(args.version)[2] == '0':
    chromedriver = 'chromedriver-{0}-{1}-{2}.zip'.format(
        get_chromedriver_version(), get_platform_key(), get_target_arch())
    upload_electron(github, release, os.path.join(DIST_DIR, chromedriver))
    mksnapshot = 'mksnapshot-{0}-{1}-{2}.zip'.format(
        ELECTRON_VERSION, get_platform_key(), get_target_arch())
    upload_electron(github, release, os.path.join(DIST_DIR, mksnapshot))

  if PLATFORM == 'win32' and not tag_exists:
    # Upload PDBs to Windows symbol server.
    run_python_script('upload-windows-pdb.py')

    # Upload node headers.
    run_python_script('upload-node-headers.py', '-v', args.version)


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      default=ELECTRON_VERSION)
  parser.add_argument('-p', '--publish-release',
                      help='Publish the release',
                      action='store_true')
  return parser.parse_args()


def run_python_script(script, *args):
  script_path = os.path.join(SOURCE_ROOT, 'script', script),
  return execute([sys.executable, script_path] + args)


def get_electron_build_version():
  if get_target_arch() == 'arm' or os.environ.has_key('CI'):
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
  name = '{0} {1}'.format(PROJECT_NAME, tag)
  if os.environ.has_key('CI'):
    body = '(placeholder)'
  else:
    body = get_text_with_editor(name)
  if body == '':
    sys.stderr.write('Quit due to empty release note.\n')
    sys.exit(0)

  data = dict(tag_name=tag, name=name, body=body, draft=True)
  r = github.repos(ELECTRON_REPO).releases.post(data=data)
  return r


def release_electron_checksums(github, release):
  checksums = run_python_script(
      'merge-electron-checksums.py', '-v', ELECTRON_VERSION)
  upload_io_to_github(github, release,
      'SHASUMS256.txt', StringIO(checksums), 'text/plain')


def upload_electron(github, release, file_path):
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
  name = os.path.dirname(file_path)
  with open(file_path, 'rb') as f:
    upload_io_to_github(github, release, name, f, 'application/zip')


def upload_io_to_github(github, release, name, io, content_type):
  params = {'name': name}
  headers = {'Content-Type': content_type}
  github.repos(ELECTRON_REPO).releases(release['id']).assets.post(
      params=params, headers=headers, data=io, verify=False)


def publish_release(github, release_id):
  data = dict(draft=False)
  github.repos(ELECTRON_REPO).releases(release_id).patch(data=data)


def auth_token():
  token = get_env_var('GITHUB_TOKEN')
  message = ('Error: Please set the $ELECTRON_GITHUB_TOKEN '
             'environment variable, which is your personal token')
  assert token, message
  return token


if __name__ == '__main__':
  import sys
  sys.exit(main())
