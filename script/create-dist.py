#!/usr/bin/env python

import os
import re
import shutil
import subprocess
import sys
import stat

from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, TARGET_PLATFORM, \
                       DIST_ARCH
from lib.util import scoped_cwd, rm_rf, get_atom_shell_version, make_zip, \
                     execute, get_chromedriver_version


ATOM_SHELL_VERSION = get_atom_shell_version()

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
CHROMIUM_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                            'download', 'libchromiumcontent', 'static_library')

TARGET_BINARIES = {
  'darwin': [
  ],
  'win32': [
    'atom.exe',
    'boringssl.dll',
    'content_shell.pak',
    'd3dcompiler_47.dll',
    'ffmpegsumo.dll',
    'icudtl.dat',
    'libEGL.dll',
    'libGLESv2.dll',
    'content_resources_200_percent.pak',
    'ui_resources_200_percent.pak',
    'xinput1_3.dll',
    'natives_blob.bin',
    'snapshot_blob.bin',
  ],
  'linux': [
    'atom',
    'content_shell.pak',
    'icudtl.dat',
    'libboringssl.so',
    'libffmpegsumo.so',
    'natives_blob.bin',
    'snapshot_blob.bin',
  ],
}
TARGET_DIRECTORIES = {
  'darwin': [
    'Atom.app',
  ],
  'win32': [
    'resources',
    'locales',
  ],
  'linux': [
    'resources',
    'locales',
  ],
}

SYSTEM_LIBRARIES = [
  'libgcrypt.so',
  'libnotify.so',
]


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  force_build()
  create_symbols()
  copy_binaries()
  copy_chromedriver()
  copy_license()

  if TARGET_PLATFORM == 'linux':
    copy_system_libraries()

  create_version()
  create_dist_zip()
  create_chromedriver_zip()
  create_symbols_zip()


def force_build():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  execute([sys.executable, build, '-c', 'Release'])


def copy_binaries():
  for binary in TARGET_BINARIES[TARGET_PLATFORM]:
    shutil.copy2(os.path.join(OUT_DIR, binary), DIST_DIR)

  for directory in TARGET_DIRECTORIES[TARGET_PLATFORM]:
    shutil.copytree(os.path.join(OUT_DIR, directory),
                    os.path.join(DIST_DIR, directory),
                    symlinks=True)


def copy_chromedriver():
  if TARGET_PLATFORM == 'win32':
    chromedriver = 'chromedriver.exe'
  else:
    chromedriver = 'chromedriver'
  src = os.path.join(CHROMIUM_DIR, chromedriver)
  dest = os.path.join(DIST_DIR, chromedriver)

  # Copy file and keep the executable bit.
  shutil.copyfile(src, dest)
  os.chmod(dest, os.stat(dest).st_mode | stat.S_IEXEC)

  # Fix the linking with boringssl.
  if TARGET_PLATFORM == 'linux':
    execute(['chrpath', '-r', '$ORIGIN', dest])
  elif TARGET_PLATFORM == 'darwin':
    shutil.copy2(os.path.join(CHROMIUM_DIR, 'libboringssl.dylib'), DIST_DIR)
    execute(['install_name_tool', '-change',
             '/usr/local/lib/libboringssl.dylib',
             '@loader_path/libboringssl.dylib',
             dest])


def copy_license():
  shutil.copy2(os.path.join(SOURCE_ROOT, 'LICENSE'), DIST_DIR)


def copy_system_libraries():
  ldd = execute(['ldd', os.path.join(OUT_DIR, 'atom')])
  lib_re = re.compile('\t(.*) => (.+) \(.*\)$')
  for line in ldd.splitlines():
    m = lib_re.match(line)
    if not m:
      continue
    for i, library in enumerate(SYSTEM_LIBRARIES):
      real_library = m.group(1)
      if real_library.startswith(library):
        shutil.copyfile(m.group(2), os.path.join(DIST_DIR, real_library))
        SYSTEM_LIBRARIES[i] = real_library


def create_version():
  version_path = os.path.join(SOURCE_ROOT, 'dist', 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(ATOM_SHELL_VERSION)


def create_symbols():
  destination = os.path.join(DIST_DIR, 'Atom-Shell.breakpad.syms')
  dump_symbols = os.path.join(SOURCE_ROOT, 'script', 'dump-symbols.py')
  execute([sys.executable, dump_symbols, destination])


def create_dist_zip():
  dist_name = 'atom-shell-{0}-{1}-{2}.zip'.format(ATOM_SHELL_VERSION,
                                                  TARGET_PLATFORM, DIST_ARCH)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = TARGET_BINARIES[TARGET_PLATFORM] +  ['LICENSE', 'version']
    if TARGET_PLATFORM == 'linux':
      files += [lib for lib in SYSTEM_LIBRARIES if os.path.exists(lib)]
    dirs = TARGET_DIRECTORIES[TARGET_PLATFORM]
    make_zip(zip_file, files, dirs)


def create_chromedriver_zip():
  dist_name = 'chromedriver-{0}-{1}-{2}.zip'.format(get_chromedriver_version(),
                                                    TARGET_PLATFORM, DIST_ARCH)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE']
    if TARGET_PLATFORM == 'win32':
      files += ['chromedriver.exe', 'boringssl.dll']
    elif TARGET_PLATFORM == 'darwin':
      files += ['chromedriver', 'libboringssl.dylib']
    elif TARGET_PLATFORM == 'linux':
      files += ['chromedriver', 'libboringssl.so']
    make_zip(zip_file, files, [])


def create_symbols_zip():
  dist_name = 'atom-shell-{0}-{1}-{2}-symbols.zip'.format(ATOM_SHELL_VERSION,
                                                          TARGET_PLATFORM,
                                                          DIST_ARCH)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE', 'version']
    dirs = ['Atom-Shell.breakpad.syms']
    make_zip(zip_file, files, dirs)


if __name__ == '__main__':
  sys.exit(main())
