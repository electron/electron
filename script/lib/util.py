#!/usr/bin/env python

import atexit
import errno
import shutil
import tarfile
import tempfile
import urllib2
import os


def tempdir(prefix=''):
  directory = tempfile.mkdtemp(prefix=prefix)
  atexit.register(shutil.rmtree, directory)
  return directory


def download(text, url, path):
  with open(path, 'w') as local_file:
    web_file = urllib2.urlopen(url)
    file_size = int(web_file.info().getheaders("Content-Length")[0])
    downloaded_size = 0
    block_size = 128

    while True:
      buf = web_file.read(block_size)
      if not buf:
        break

      downloaded_size += len(buf)
      local_file.write(buf)

      percent = downloaded_size * 100. / file_size
      status = "\r%s  %10d  [%3.1f%%]" % (text, downloaded_size, percent)
      print status,

    print


def extract_tarball(tarball_path, member, path):
  with tarfile.open(tarball_path) as tarball:
    tarball.extract(member, path)


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
