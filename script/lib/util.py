#!/usr/bin/env python3

from __future__ import print_function
import atexit
import contextlib
import errno
import json
import os
import shutil
import ssl
import stat
import subprocess
import sys
import tarfile
import tempfile
# Python 3 / 2 compat import
try:
  from urllib.request import urlopen
except ImportError:
  from urllib2 import urlopen
import zipfile

from lib.config import is_verbose_mode

ELECTRON_DIR = os.path.abspath(
  os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
)
TS_NODE = os.path.join(ELECTRON_DIR, 'node_modules', '.bin', 'ts-node')
SRC_DIR = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..'))

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'
  TS_NODE += '.cmd'


@contextlib.contextmanager
def scoped_cwd(path):
  cwd = os.getcwd()
  os.chdir(path)
  try:
    yield
  finally:
    os.chdir(cwd)


@contextlib.contextmanager
def scoped_env(key, value):
  origin = ''
  if key in os.environ:
    origin = os.environ[key]
  os.environ[key] = value
  try:
    yield
  finally:
    os.environ[key] = origin


def download(text, url, path):
  safe_mkdir(os.path.dirname(path))
  with open(path, 'wb') as local_file:
    print("Downloading %s to %s" % (url, path))
    web_file = urlopen(url)
    info = web_file.info()
    if hasattr(info, 'getheader'):
      file_size = int(info.getheaders("Content-Length")[0])
    else:
      file_size = int(info.get("Content-Length")[0])
    downloaded_size = 0
    block_size = 4096

    ci = os.environ.get('CI') is not None

    while True:
      buf = web_file.read(block_size)
      if not buf:
        break

      downloaded_size += len(buf)
      local_file.write(buf)

      if not ci:
        percent = downloaded_size * 100. / file_size
        status = "\r%s  %10d  [%3.1f%%]" % (text, downloaded_size, percent)
        print(status, end=' ')

    if ci:
      print("%s done." % (text))
    else:
      print()
  return path


def make_zip(zip_file_path, files, dirs):
  safe_unlink(zip_file_path)
  if sys.platform == 'darwin':
    allfiles = files + dirs
    execute(['zip', '-r', '-y', zip_file_path] + allfiles)
  else:
    zip_file = zipfile.ZipFile(zip_file_path, "w", zipfile.ZIP_DEFLATED,
                               allowZip64=True)
    for filename in files:
      zip_file.write(filename, filename)
    for dirname in dirs:
      for root, _, filenames in os.walk(dirname):
        for f in filenames:
          zip_file.write(os.path.join(root, f))
    zip_file.close()


def rm_rf(path):
  try:
    shutil.rmtree(path)
  except OSError:
    pass


def safe_unlink(path):
  try:
    os.unlink(path)
  except OSError as e:
    if e.errno != errno.ENOENT:
      raise


def safe_mkdir(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise


def execute(argv, env=None, cwd=None):
  if env is None:
    env = os.environ
  if is_verbose_mode():
    print(' '.join(argv))
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT,
                                     env=env, cwd=cwd)
    if is_verbose_mode():
      print(output)
    return output
  except subprocess.CalledProcessError as e:
    print(e.output)
    raise e


def get_electron_branding():
  SOURCE_ROOT = os.path.abspath(os.path.join(__file__, '..', '..', '..'))
  branding_file_path = os.path.join(
    SOURCE_ROOT, 'shell', 'app', 'BRANDING.json')
  with open(branding_file_path) as f:
    return json.load(f)

def get_electron_version():
  SOURCE_ROOT = os.path.abspath(os.path.join(__file__, '..', '..', '..'))
  version_file = os.path.join(SOURCE_ROOT, 'ELECTRON_VERSION')
  with open(version_file) as f:
    return 'v' + f.read().strip()

def s3put(bucket, access_key, secret_key, prefix, key_prefix, files):
  env = os.environ.copy()
  env['AWS_ACCESS_KEY_ID'] = access_key
  env['AWS_SECRET_ACCESS_KEY'] = secret_key
  output = execute([
    'node',
    os.path.join(os.path.dirname(__file__), 's3put.js'),
    '--bucket', bucket,
    '--prefix', prefix,
    '--key_prefix', key_prefix,
    '--grant', 'public-read',
  ] + files, env)
  print(output)

def get_out_dir():
  out_dir = 'Debug'
  override = os.environ.get('ELECTRON_OUT_DIR')
  if override is not None:
    out_dir = override
  return os.path.join(SRC_DIR, 'out', out_dir)

# NOTE: This path is not created by gn, it is used as a scratch zone by our
#       upload scripts
def get_dist_dir():
  return os.path.join(get_out_dir(), 'gen', 'electron_dist')

def get_electron_exec():
  out_dir = get_out_dir()

  if sys.platform == 'darwin':
    return '{0}/Electron.app/Contents/MacOS/Electron'.format(out_dir)
  elif sys.platform == 'win32':
    return '{0}/electron.exe'.format(out_dir)
  elif sys.platform == 'linux':
    return '{0}/electron'.format(out_dir)

  raise Exception(
      "get_electron_exec: unexpected platform '{0}'".format(sys.platform))

def get_buildtools_executable(name):
  buildtools = os.path.realpath(os.path.join(ELECTRON_DIR, '..', 'buildtools'))
  chromium_platform = {
    'darwin': 'mac',
    'linux': 'linux64',
    'linux2': 'linux64',
    'win32': 'win',
  }[sys.platform]
  path = os.path.join(buildtools, chromium_platform, name)
  if sys.platform == 'win32':
    path += '.exe'
  return path
