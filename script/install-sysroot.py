#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to install a Debian Wheezy sysroot for making official Google Chrome
# Linux builds.
# The sysroot is needed to make Chrome work for Debian Wheezy.
# This script can be run manually but is more often run as part of gclient
# hooks. When run from hooks this script should be a no-op on non-linux
# platforms.

# The sysroot image could be constructed from scratch based on the current
# state or Debian Wheezy but for consistency we currently use a pre-built root
# image. The image will normally need to be rebuilt every time chrome's build
# dependancies are changed.

import hashlib
import platform
import optparse
import os
import re
import shutil
import subprocess
import sys

from lib.util import get_host_arch


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
URL_PREFIX = 'https://github.com'
URL_PATH = 'atom/debian-sysroot-image-creator/releases/download'
REVISION_AMD64 = 264817
REVISION_I386 = 'v0.2.0'
REVISION_ARM = 'v0.1.0'
TARBALL_AMD64 = 'debian_wheezy_amd64_sysroot.tgz'
TARBALL_I386 = 'debian_wheezy_i386_sysroot.tgz'
TARBALL_ARM = 'debian_wheezy_arm_sysroot.tgz'
TARBALL_AMD64_SHA1SUM = '74b7231e12aaf45c5c5489d9aebb56bd6abb3653'
TARBALL_I386_SHA1SUM = 'f5b2ceaeb3f7e6bc2058733585fe877d002b5fa7'
TARBALL_ARM_SHA1SUM = '72e668c57b8591e108759584942ddb6f6cee1322'
SYSROOT_DIR_AMD64 = 'debian_wheezy_amd64-sysroot'
SYSROOT_DIR_I386 = 'debian_wheezy_i386-sysroot'
SYSROOT_DIR_ARM = 'debian_wheezy_arm-sysroot'

valid_archs = ('arm', 'i386', 'amd64')


def GetSha1(filename):
  sha1 = hashlib.sha1()
  with open(filename, 'rb') as f:
    while True:
      # Read in 1mb chunks, so it doesn't all have to be loaded into memory.
      chunk = f.read(1024*1024)
      if not chunk:
        break
      sha1.update(chunk)
  return sha1.hexdigest()


def DetectArch(gyp_defines):
  # Check for optional target_arch and only install for that architecture.
  # If target_arch is not specified, then only install for the host
  # architecture.
  if 'target_arch=x64' in gyp_defines:
    return 'amd64'
  elif 'target_arch=ia32' in gyp_defines:
    return 'i386'
  elif 'target_arch=arm' in gyp_defines:
    return 'arm'

  detected_host_arch = get_host_arch()
  if detected_host_arch == 'x64':
    return 'amd64'
  elif detected_host_arch == 'ia32':
    return 'i386'
  elif detected_host_arch == 'arm':
    return 'arm'
  else:
    print "Unknown host arch: %s" % detected_host_arch

  return None


def main():
  if options.linux_only:
    # This argument is passed when run from the gclient hooks.
    # In this case we return early on non-linux platforms.
    if not sys.platform.startswith('linux'):
      return 0

  gyp_defines = os.environ.get('GYP_DEFINES', '')

  if options.arch:
    target_arch = options.arch
  else:
    target_arch = DetectArch(gyp_defines)
    if not target_arch:
      print 'Unable to detect host architecture'
      return 1

  if options.linux_only and target_arch != 'arm':
    # When run from runhooks, only install the sysroot for an Official Chrome
    # Linux build, except on ARM where we always use a sysroot.
    defined = ['branding=Chrome', 'buildtype=Official']
    undefined = ['chromeos=1']
    for option in defined:
      if option not in gyp_defines:
        return 0
    for option in undefined:
      if option in gyp_defines:
        return 0

  # The sysroot directory should match the one specified in build/common.gypi.
  # TODO(thestig) Consider putting this else where to avoid having to recreate
  # it on every build.
  linux_dir = os.path.join(SCRIPT_DIR, '..', 'vendor')
  if target_arch == 'amd64':
    sysroot = os.path.join(linux_dir, SYSROOT_DIR_AMD64)
    tarball_filename = TARBALL_AMD64
    tarball_sha1sum = TARBALL_AMD64_SHA1SUM
    revision = REVISION_AMD64
  elif target_arch == 'arm':
    sysroot = os.path.join(linux_dir, SYSROOT_DIR_ARM)
    tarball_filename = TARBALL_ARM
    tarball_sha1sum = TARBALL_ARM_SHA1SUM
    revision = REVISION_ARM
  elif target_arch == 'i386':
    sysroot = os.path.join(linux_dir, SYSROOT_DIR_I386)
    tarball_filename = TARBALL_I386
    tarball_sha1sum = TARBALL_I386_SHA1SUM
    revision = REVISION_I386
  else:
    print 'Unknown architecture: %s' % target_arch
    assert(False)

  url = '%s/%s/%s/%s' % (URL_PREFIX, URL_PATH, revision, tarball_filename)

  stamp = os.path.join(sysroot, '.stamp')
  if os.path.exists(stamp):
    with open(stamp) as s:
      if s.read() == url:
        print 'Debian Wheezy %s root image already up-to-date: %s' % \
            (target_arch, sysroot)
        return 0

  print 'Installing Debian Wheezy %s root image: %s' % (target_arch, sysroot)
  if os.path.isdir(sysroot):
    shutil.rmtree(sysroot)
  os.mkdir(sysroot)
  tarball = os.path.join(sysroot, tarball_filename)
  print 'Downloading %s' % url
  sys.stdout.flush()
  sys.stderr.flush()
  subprocess.check_call(['curl', '--fail', '-L', url, '-o', tarball])
  sha1sum = GetSha1(tarball)
  if sha1sum != tarball_sha1sum:
    print 'Tarball sha1sum is wrong.'
    print 'Expected %s, actual: %s' % (tarball_sha1sum, sha1sum)
    return 1
  subprocess.check_call(['tar', 'xf', tarball, '-C', sysroot])
  os.remove(tarball)

  with open(stamp, 'w') as s:
    s.write(url)
  return 0


if __name__ == '__main__':
  parser = optparse.OptionParser('usage: %prog [OPTIONS]')
  parser.add_option('--linux-only', action='store_true',
                    default=False, help='Only install sysroot for official '
                                        'Linux builds')
  parser.add_option('--arch', type='choice', choices=valid_archs,
                    help='Sysroot architecture: %s' % ', '.join(valid_archs))
  options, _ = parser.parse_args()
  sys.exit(main())
