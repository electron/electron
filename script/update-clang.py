#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is used to download prebuilt clang binaries."""

import os
import shutil
import subprocess
import stat
import sys
import tarfile
import tempfile
import time
import urllib2


# CLANG_REVISION and CLANG_SUB_REVISION determine the build of clang
# to use. These should be synced with tools/clang/scripts/update.py in
# Chromium.
CLANG_REVISION = '325667'
CLANG_SUB_REVISION=1

PACKAGE_VERSION = "%s-%s" % (CLANG_REVISION, CLANG_SUB_REVISION)

# Path constants. (All of these should be absolute paths.)
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
LLVM_DIR = os.path.join(THIS_DIR, '..', 'vendor', 'llvm-build')
LLVM_BUILD_DIR = os.path.realpath(LLVM_DIR)
STAMP_FILE = os.path.join(LLVM_BUILD_DIR, 'cr_build_revision')

# URL for pre-built binaries.
CDS_URL = os.environ.get('CDS_CLANG_BUCKET_OVERRIDE',
    'https://commondatastorage.googleapis.com/chromium-browser-clang')

# Bump after VC updates.
DIA_DLL = {
  '2013': 'msdia120.dll',
  '2015': 'msdia140.dll',
  '2017': 'msdia140.dll',
}


def DownloadUrl(url, output_file):
  """Download url into output_file."""
  CHUNK_SIZE = 4096
  TOTAL_DOTS = 10
  num_retries = 3
  retry_wait_s = 5  # Doubled at each retry.

  while True:
    try:
      sys.stdout.write('Downloading %s ' % url)
      sys.stdout.flush()
      response = urllib2.urlopen(url)
      total_size = int(response.info().getheader('Content-Length').strip())
      bytes_done = 0
      dots_printed = 0
      while True:
        chunk = response.read(CHUNK_SIZE)
        if not chunk:
          break
        output_file.write(chunk)
        bytes_done += len(chunk)
        num_dots = TOTAL_DOTS * bytes_done / total_size
        sys.stdout.write('.' * (num_dots - dots_printed))
        sys.stdout.flush()
        dots_printed = num_dots
      if bytes_done != total_size:
        raise urllib2.URLError("only got %d of %d bytes" %
                               (bytes_done, total_size))
      print ' Done.'
      return
    except urllib2.URLError as e:
      sys.stdout.write('\n')
      print e
      if num_retries == 0 or isinstance(e, urllib2.HTTPError) and e.code == 404:
        raise e
      num_retries -= 1
      print 'Retrying in %d s ...' % retry_wait_s
      time.sleep(retry_wait_s)
      retry_wait_s *= 2


def EnsureDirExists(path):
  if not os.path.exists(path):
    print "Creating directory %s" % path
    os.makedirs(path)


def DownloadAndUnpack(url, output_dir):
  with tempfile.TemporaryFile() as f:
    DownloadUrl(url, f)
    f.seek(0)
    EnsureDirExists(output_dir)
    tarfile.open(mode='r:gz', fileobj=f).extractall(path=output_dir)


def ReadStampFile(path=STAMP_FILE):
  """Return the contents of the stamp file, or '' if it doesn't exist."""
  try:
    with open(path, 'r') as f:
      return f.read().rstrip()
  except IOError:
    return ''


def WriteStampFile(s, path=STAMP_FILE):
  """Write s to the stamp file."""
  EnsureDirExists(os.path.dirname(path))
  with open(path, 'w') as f:
    f.write(s)
    f.write('\n')


def RmTree(directoryPath):
  """Delete dir."""
  def ChmodAndRetry(func, path, _):
    # Subversion can leave read-only files around.
    if not os.access(path, os.W_OK):
      os.chmod(path, stat.S_IWUSR)
      return func(path)
    raise

  shutil.rmtree(directoryPath, onerror=ChmodAndRetry)


def CopyFile(src, dst):
  """Copy a file from src to dst."""
  print "Copying %s to %s" % (src, dst)
  shutil.copy(src, dst)


vs_version = None
def GetVSVersion():
  global vs_version
  if vs_version:
    return vs_version

  # Try using the toolchain in depot_tools.
  # This sets environment variables used by SelectVisualStudioVersion below.
  sys.path.append(THIS_DIR)
  import vs_toolchain
  vs_toolchain.SetEnvironmentAndGetRuntimeDllDirs()

  # Use gyp to find the MSVS installation, either in depot_tools as per above,
  # or a system-wide installation otherwise.
  sys.path.append(os.path.join(THIS_DIR, 'gyp', 'pylib'))
  import gyp.MSVSVersion
  vs_version = gyp.MSVSVersion.SelectVisualStudioVersion(
      vs_toolchain.GetVisualStudioVersion())
  return vs_version

def ValidVS2017Path():
  vs2017Path = os.path.join(os.environ['ProgramFiles(x86)'], \
                            'Microsoft Visual Studio', '2017')
  vs2017Variants = ['Enterprise', 'Professional', 'Community']

  for vs2017Variant in vs2017Variants:
    possiblePath = os.path.join(vs2017Path, vs2017Variant)
    if os.path.exists(possiblePath):
      return possiblePath

  return ''


def ValidVS2015Path():
  vs2015Path = os.path.join(os.environ['ProgramFiles(x86)'], \
                            'Microsoft Visual Studio 14.0')
  if os.path.exists(vs2015Path):
    return vs2015Path
  return ''


def ValidVSPath():
  result = ValidVS2017Path()
  if (len(result) == 0):
    result = ValidVS2015Path()
  if (len(result) == 0):
    raise IOError
  
  return result


def CopyDiaDllTo(target_dir):
  # This script always wants to use the 64-bit msdia*.dll.
  vsPath = ValidVSPath()
  msdia140Path = os.path.join('DIA SDK', 'bin', 'amd64', 'msdia140.dll')

  dia_dll_path = os.path.join(vsPath, msdia140Path)
  if not os.path.exists(dia_dll_path):
    raise IOError

  dia_dll =  dia_dll_path
  CopyFile(dia_dll, target_dir)


def UpdateClang():
  cds_file = "clang-%s.tgz" %  PACKAGE_VERSION
  if sys.platform == 'win32' or sys.platform == 'cygwin':
    cds_full_url = CDS_URL + '/Win/' + cds_file
  elif sys.platform.startswith('linux'):
    cds_full_url = CDS_URL + '/Linux_x64/' + cds_file
  else:
    return 0

  print 'Updating Clang to %s...' % PACKAGE_VERSION

  if ReadStampFile() == PACKAGE_VERSION:
    print 'Clang is already up to date.'
    return 0

  # Reset the stamp file in case the build is unsuccessful.
  WriteStampFile('')

  print 'Downloading prebuilt clang'
  if os.path.exists(LLVM_BUILD_DIR):
    RmTree(LLVM_BUILD_DIR)
  try:
    DownloadAndUnpack(cds_full_url, LLVM_BUILD_DIR)
    print 'clang %s unpacked' % PACKAGE_VERSION
    if sys.platform == 'win32':
      CopyDiaDllTo(os.path.join(LLVM_BUILD_DIR, 'bin'))
    WriteStampFile(PACKAGE_VERSION)
    return 0
  except urllib2.URLError:
    print 'Failed to download prebuilt clang %s' % cds_file
    print 'Exiting.'
    return 1


def main():
  # Don't buffer stdout, so that print statements are immediately flushed.
  sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
  return UpdateClang()


if __name__ == '__main__':
  sys.exit(main())
