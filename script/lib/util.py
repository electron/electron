#!/usr/bin/env python

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
import urllib2
import zipfile

from lib.config import is_verbose_mode, PLATFORM
from lib.env_util import get_vs_env

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

    print "Downloading %s to %s" % (url, path)
    web_file = urllib2.urlopen(url)
    file_size = int(web_file.info().getheaders("Content-Length")[0])
    downloaded_size = 0
    block_size = 128

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
        print status,

    if ci:
      print "%s done." % (text)
    else:
      print
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
    zip_file = zipfile.ZipFile(zip_file_path, "w", zipfile.ZIP_DEFLATED)
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
    print ' '.join(argv)
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT,
                                     env=env, cwd=cwd)
    if is_verbose_mode():
      print output
    return output
  except subprocess.CalledProcessError as e:
    print e.output
    raise e


def execute_stdout(argv, env=None, cwd=None):
  if env is None:
    env = os.environ
  if is_verbose_mode():
    print ' '.join(argv)
    try:
      subprocess.check_call(argv, env=env, cwd=cwd)
    except subprocess.CalledProcessError as e:
      print e.output
      raise e
  else:
    execute(argv, env, cwd)

def get_electron_branding():
  SOURCE_ROOT = os.path.abspath(os.path.join(__file__, '..', '..', '..'))
  branding_file_path = os.path.join(SOURCE_ROOT, 'atom', 'app', 'BRANDING.json')
  with open(branding_file_path) as f:
    return json.load(f)

def get_electron_version():
  SOURCE_ROOT = os.path.abspath(os.path.join(__file__, '..', '..', '..'))
  version_file = os.path.join(SOURCE_ROOT, 'VERSION')
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

def parse_version(version):
  if version[0] == 'v':
    version = version[1:]

  vs = version.split('.')
  if len(vs) > 4:
    return vs[0:4]
  else:
    return vs + ['0'] * (4 - len(vs))

def clean_parse_version(v):
  return parse_version(v.split("-")[0])        

def is_stable(v):
  return len(v.split(".")) == 3    

def is_beta(v):
  return 'beta' in v

def is_nightly(v):
  return 'nightly' in v

def get_nightly_date():
  return datetime.datetime.today().strftime('%Y%m%d')

def get_last_major():
  return execute(['node', 'script/get-last-major-for-master.js'])

def get_next_nightly(v):
  pv = clean_parse_version(v)
  (major, minor, patch) = pv[0:3]

  if (is_stable(v)):
    patch = str(int(pv[2]) + 1)

  if execute(['git', 'rev-parse', '--abbrev-ref', 'HEAD']) == "master":
    major = str(get_last_major() + 1)
    minor = '0'
    patch = '0'

  pre = 'nightly.' + get_nightly_date()
  return make_version(major, minor, patch, pre)

def non_empty(thing):
  return thing.strip() != ''

def beta_tag_compare(tag1, tag2):
  p1 = parse_version(tag1)
  p2 = parse_version(tag2)
  return int(p1[3]) - int(p2[3])

def get_next_beta(v):
  pv = clean_parse_version(v)
  tag_pattern = 'v' + pv[0] + '.' + pv[1] + '.' + pv[2] + '-beta.*'
  tag_list = sorted(filter(
    non_empty,
    execute(['git', 'tag', '--list', '-l', tag_pattern]).strip().split('\n')
  ), cmp=beta_tag_compare)
  if len(tag_list) == 0:
    return make_version(pv[0] , pv[1],  pv[2], 'beta.1')

  lv = parse_version(tag_list[-1])
  return make_version(pv[0] , pv[1],  pv[2], 'beta.' + str(int(lv[3]) + 1))

def get_next_stable_from_pre(v):
  pv = clean_parse_version(v)
  (major, minor, patch) = pv[0:3]
  return make_version(major, minor, patch)

def get_next_stable_from_stable(v):
  pv = clean_parse_version(v)
  (major, minor, patch) = pv[0:3]
  return make_version(major, minor, str(int(patch) + 1))

def make_version(major, minor, patch, pre = None):
  if pre is None:
    return major + '.' + minor + '.' + patch
  return major + "." + minor + "." + patch + '-' + pre

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
