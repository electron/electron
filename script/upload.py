#!/usr/bin/env python

import argparse
import errno
import os
import subprocess
import sys
import tempfile

from lib.config import PLATFORM, get_target_arch, get_chromedriver_version, \
                       get_platform_key
from lib.util import atom_gyp, execute, get_atom_shell_version, parse_version, \
                     scoped_cwd
from lib.github import GitHub


ATOM_SHELL_REPO = 'atom/electron'
ATOM_SHELL_VERSION = get_atom_shell_version()

PROJECT_NAME = atom_gyp()['project_name%']
PRODUCT_NAME = atom_gyp()['product_name%']

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
DIST_NAME = '{0}-{1}-{2}-{3}.zip'.format(PROJECT_NAME,
                                         ATOM_SHELL_VERSION,
                                         get_platform_key(),
                                         get_target_arch())
SYMBOLS_NAME = '{0}-{1}-{2}-{3}-symbols.zip'.format(PROJECT_NAME,
                                                    ATOM_SHELL_VERSION,
                                                    get_platform_key(),
                                                    get_target_arch())
MKSNAPSHOT_NAME = 'mksnapshot-{0}-{1}-{2}.zip'.format(ATOM_SHELL_VERSION,
                                                      get_platform_key(),
                                                      get_target_arch())


def main():
  args = parse_args()

  if not args.publish_release:
    if not dist_newer_than_head():
      create_dist = os.path.join(SOURCE_ROOT, 'script', 'create-dist.py')
      execute([sys.executable, create_dist])

    build_version = get_atom_shell_build_version()
    if not ATOM_SHELL_VERSION.startswith(build_version):
      error = 'Tag name ({0}) should match build version ({1})\n'.format(
          ATOM_SHELL_VERSION, build_version)
      sys.stderr.write(error)
      sys.stderr.flush()
      return 1

  github = GitHub(auth_token())
  releases = github.repos(ATOM_SHELL_REPO).releases.get()
  tag_exists = False
  for release in releases:
    if not release['draft'] and release['tag_name'] == args.version:
      tag_exists = True
      break

  release = create_or_get_release_draft(github, releases, args.version,
                                        tag_exists)

  if args.publish_release:
    # Upload the SHASUMS.txt.
    execute([sys.executable,
             os.path.join(SOURCE_ROOT, 'script', 'upload-checksums.py'),
             '-v', ATOM_SHELL_VERSION])

    # Upload the index.json.
    execute([sys.executable,
             os.path.join(SOURCE_ROOT, 'script', 'upload-index-json.py')])

    # Press the publish button.
    publish_release(github, release['id'])

    # Do not upload other files when passed "-p".
    return

  # Upload atom-shell with GitHub Releases API.
  upload_atom_shell(github, release, os.path.join(DIST_DIR, DIST_NAME))
  upload_atom_shell(github, release, os.path.join(DIST_DIR, SYMBOLS_NAME))

  # Upload chromedriver and mksnapshot for minor version update.
  if parse_version(args.version)[2] == '0':
    chromedriver = 'chromedriver-{0}-{1}-{2}.zip'.format(
        get_chromedriver_version(), get_platform_key(), get_target_arch())
    upload_atom_shell(github, release, os.path.join(DIST_DIR, chromedriver))
    upload_atom_shell(github, release, os.path.join(DIST_DIR, MKSNAPSHOT_NAME))

  if PLATFORM == 'win32' and not tag_exists:
    # Upload node headers.
    execute([sys.executable,
             os.path.join(SOURCE_ROOT, 'script', 'upload-node-headers.py'),
             '-v', args.version])


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      default=ATOM_SHELL_VERSION)
  parser.add_argument('-p', '--publish-release',
                      help='Publish the release',
                      action='store_true')
  return parser.parse_args()


def get_atom_shell_build_version():
  if get_target_arch() == 'arm' or os.environ.has_key('CI'):
    # In CI we just build as told.
    return ATOM_SHELL_VERSION
  if PLATFORM == 'darwin':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'R',
                              '{0}.app'.format(PRODUCT_NAME), 'Contents',
                              'MacOS', PRODUCT_NAME)
  elif PLATFORM == 'win32':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'R',
                              '{0}.exe'.format(PROJECT_NAME))
  else:
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'R', PROJECT_NAME)

  return subprocess.check_output([atom_shell, '--version']).strip()


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
  if os.environ.has_key('CI'):
    name = '{0} pending draft'.format(PROJECT_NAME)
    body = '(placeholder)'
  else:
    name = '{0} {1}'.format(PROJECT_NAME, tag)
    body = get_text_with_editor(name)
  if body == '':
    sys.stderr.write('Quit due to empty release note.\n')
    sys.exit(0)

  data = dict(tag_name=tag, name=name, body=body, draft=True)
  r = github.repos(ATOM_SHELL_REPO).releases.post(data=data)
  return r


def upload_atom_shell(github, release, file_path):
  # Delete the original file before uploading in CI.
  if os.environ.has_key('CI'):
    try:
      for asset in release['assets']:
        if asset['name'] == os.path.basename(file_path):
          github.repos(ATOM_SHELL_REPO).releases.assets(asset['id']).delete()
          break
    except Exception:
      pass

  # Upload the file.
  params = {'name': os.path.basename(file_path)}
  headers = {'Content-Type': 'application/zip'}
  with open(file_path, 'rb') as f:
    github.repos(ATOM_SHELL_REPO).releases(release['id']).assets.post(
        params=params, headers=headers, data=f, verify=False)


def publish_release(github, release_id):
  data = dict(draft=False)
  github.repos(ATOM_SHELL_REPO).releases(release_id).patch(data=data)


def auth_token():
  token = os.environ.get('ATOM_SHELL_GITHUB_TOKEN')
  message = ('Error: Please set the $ATOM_SHELL_GITHUB_TOKEN '
             'environment variable, which is your personal token')
  assert token, message
  return token


if __name__ == '__main__':
  import sys
  sys.exit(main())
