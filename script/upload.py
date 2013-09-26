#!/usr/bin/env python

import argparse
import errno
import glob
import os
import requests
import subprocess
import sys
import tempfile

from lib.util import *
from lib.github import GitHub


TARGET_PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

ATOM_SHELL_REPO = 'atom/atom-shell'
ATOM_SHELL_VRESION = get_atom_shell_version()
NODE_VERSION = 'v0.10.18'

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'Release')
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
DIST_NAME = 'atom-shell-{0}-{1}.zip'.format(ATOM_SHELL_VRESION, TARGET_PLATFORM)


def main():
  args = parse_args()

  if not dist_newer_than_head():
    create_dist = os.path.join(SOURCE_ROOT, 'script', 'create-dist.py')
    subprocess.check_output([sys.executable, create_dist])

  # Upload atom-shell with GitHub Releases API.
  github = GitHub(auth_token())
  release_id = create_or_get_release_draft(github, args.version)
  upload_atom_shell(github, release_id, os.path.join(DIST_DIR, DIST_NAME))
  if not args.no_publish_release:
    publish_release(github, release_id)

  # Upload node's headers to S3.
  bucket, access_key, secret_key = s3_config()
  upload_node(bucket, access_key, secret_key, NODE_VERSION)


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      default=ATOM_SHELL_VRESION)
  parser.add_argument('-n', '--no-publish-release',
                      help='Do not publish the release',
                      action='store_true')
  return parser.parse_args()


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


def create_or_get_release_draft(github, tag):
  name = 'atom-shell %s' % tag
  releases = github.repos(ATOM_SHELL_REPO).releases.get()
  for release in releases:
    # The untagged commit doesn't have a matching tag_name, so also check name.
    if release['tag_name'] == tag or release['name'] == name:
      return release['id']

  return create_release_draft(github, tag)


def create_release_draft(github, tag):
  name = 'atom-shell %s' % tag
  body = ''

  print 'Please enter content for the %s release note:' % name
  for line in sys.stdin:
    body += line

  data = dict(tag_name=tag, target_commitish=tag, name=name, body=body,
              draft=True)
  r = github.repos(ATOM_SHELL_REPO).releases.post(data=data)
  return r['id']


def upload_atom_shell(github, release_id, file_path):
  params = {'name': os.path.basename(file_path)}
  headers = {'Content-Type': 'application/zip'}
  files = {'file': open(file_path, 'rb')}
  github.repos(ATOM_SHELL_REPO).releases(release_id).assets.post(
      params=params, headers=headers, files=files, verify=False)


def publish_release(github, release_id):
  data = dict(draft=False)
  github.repos(ATOM_SHELL_REPO).releases(release_id).patch(data=data)


def upload_node(bucket, access_key, secret_key, version):
  os.chdir(DIST_DIR)

  s3put(bucket, access_key, secret_key, DIST_DIR,
        'atom-shell/dist/{0}'.format(version), glob.glob('node-*.tar.gz'))

  if TARGET_PLATFORM == 'win32':
    # Generate the node.lib.
    build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
    subprocess.check_output([sys.executable, build, '-c', 'Release',
                             '-t', 'generate_node_lib'])

    # Upload the 32bit node.lib.
    node_lib = os.path.join(OUT_DIR, 'node.lib')
    s3put(bucket, access_key, secret_key, OUT_DIR,
          'atom-shell/dist/{0}'.format(version), [node_lib])

    # Upload the fake 64bit node.lib.
    touch_x64_node_lib()
    node_lib = os.path.join(OUT_DIR, 'x64', 'node.lib')
    s3put(bucket, access_key, secret_key, OUT_DIR,
          'atom-shell/dist/{0}'.format(version), [node_lib])


def auth_token():
  token = os.environ.get('ATOM_SHELL_GITHUB_TOKEN')
  message = ('Error: Please set the $ATOM_SHELL_GITHUB_TOKEN '
             'environment variable, which is your personal token')
  assert token, message
  return token


def s3_config():
  config = (os.environ.get('ATOM_SHELL_S3_BUCKET', ''),
            os.environ.get('ATOM_SHELL_S3_ACCESS_KEY', ''),
            os.environ.get('ATOM_SHELL_S3_SECRET_KEY', ''))
  message = ('Error: Please set the $ATOM_SHELL_S3_BUCKET, '
             '$ATOM_SHELL_S3_ACCESS_KEY, and '
             '$ATOM_SHELL_S3_SECRET_KEY environment variables')
  assert all(len(c) for c in config), message
  return config


def s3put(bucket, access_key, secret_key, prefix, key_prefix, files):
  args = [
    's3put',
    '--bucket', bucket,
    '--access_key', access_key,
    '--secret_key', secret_key,
    '--prefix', prefix,
    '--key_prefix', key_prefix,
    '--grant', 'public-read'
  ] + files

  subprocess.check_output(args)


def touch_x64_node_lib():
  x64_dir = os.path.join(OUT_DIR, 'x64')
  safe_mkdir(x64_dir)
  with open(os.path.join(x64_dir, 'node.lib'), 'w+') as node_lib:
    node_lib.write('Invalid library')


if __name__ == '__main__':
  import sys
  sys.exit(main())
