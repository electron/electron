#!/usr/bin/env python

from __future__ import print_function
import atexit
import contextlib
import datetime
import errno
import json
import os
import platform
import re
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

from lib.config import is_verbose_mode, PLATFORM
from lib.env_util import get_vs_env

ELECTRON_DIR = os.path.abspath(
  os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
)
SRC_DIR = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..'))
BOTO_DIR = os.path.abspath(os.path.join(__file__, '..', '..', '..', 'vendor',
                                        'boto'))

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'


def tempdir(prefix=''):
  directory = tempfile.mkdtemp(prefix=prefix)
  atexit.register(shutil.rmtree, directory)
  return directory


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
    if hasattr(ssl, '_create_unverified_context'):
      ssl._create_default_https_context = ssl._create_unverified_context

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


def extract_tarball(tarball_path, member, destination):
  with tarfile.open(tarball_path) as tarball:
    tarball.extract(member, destination)


def extract_zip(zip_path, destination):
  if sys.platform == 'darwin':
    # Use unzip command on Mac to keep symbol links in zip file work.
    execute(['unzip', zip_path, '-d', destination])
  else:
    with zipfile.ZipFile(zip_path) as z:
      z.extractall(destination)

def make_zip(zip_file_path, files, dirs):
  safe_unlink(zip_file_path)
  if sys.platform == 'darwin':
    files += dirs
    execute(['zip', '-r', '-y', zip_file_path] + files)
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


def execute_stdout(argv, env=None, cwd=None):
  if env is None:
    env = os.environ
  if is_verbose_mode():
    print(' '.join(argv))
    try:
      subprocess.check_call(argv, env=env, cwd=cwd)
    except subprocess.CalledProcessError as e:
      print(e.output)
      raise e
  else:
    execute(argv, env, cwd)

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

def boto_path_dirs():
  return [
    os.path.join(BOTO_DIR, 'build', 'lib'),
    os.path.join(BOTO_DIR, 'build', 'lib.linux-x86_64-2.7')
  ]


def run_boto_script(access_key, secret_key, script_name, *args):
  env = os.environ.copy()
  env['AWS_ACCESS_KEY_ID'] = access_key
  env['AWS_SECRET_ACCESS_KEY'] = secret_key
  env['PYTHONPATH'] = os.path.pathsep.join(
      [env.get('PYTHONPATH', '')] + boto_path_dirs())

  boto = os.path.join(BOTO_DIR, 'bin', script_name)
  execute([sys.executable, boto] + list(args), env)


def s3put(bucket, access_key, secret_key, prefix, key_prefix, files):
  args = [
    '--bucket', bucket,
    '--prefix', prefix,
    '--key_prefix', key_prefix,
    '--grant', 'public-read'
  ] + files

  run_boto_script(access_key, secret_key, 's3put', *args)


def add_exec_bit(filename):
  os.chmod(filename, os.stat(filename).st_mode | stat.S_IEXEC)

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

def get_objcopy_path(target_cpu):
  if PLATFORM != 'linux':
    raise Exception(
      "get_objcopy_path: unexpected platform '{0}'".format(PLATFORM))

  if target_cpu != 'x64':
      raise Exception(
      "get_objcopy_path: unexpected target cpu '{0}'".format(target_cpu))
  return os.path.join(SRC_DIR, 'third_party', 'binutils', 'Linux_x64',
                        'Release', 'bin', 'objcopy')
