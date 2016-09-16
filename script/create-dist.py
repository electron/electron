#!/usr/bin/env python

import glob
import os
import re
import shutil
import subprocess
import sys
import stat

from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, PLATFORM, \
                       get_target_arch, get_chromedriver_version, \
                       get_zip_name
from lib.util import scoped_cwd, rm_rf, get_electron_version, make_zip, \
                     execute, electron_gyp


ELECTRON_VERSION = get_electron_version()

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
CHROMIUM_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                            'download', 'libchromiumcontent', 'static_library')

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']

TARGET_BINARIES = {
  'darwin': [
  ],
  'win32': [
    '{0}.exe'.format(PROJECT_NAME),  # 'electron.exe'
    'content_shell.pak',
    'd3dcompiler_47.dll',
    'icudtl.dat',
    'libEGL.dll',
    'libGLESv2.dll',
    'ffmpeg.dll',
    'node.dll',
    'blink_image_resources_200_percent.pak',
    'content_resources_200_percent.pak',
    'ui_resources_200_percent.pak',
    'views_resources_200_percent.pak',
    'xinput1_3.dll',
    'natives_blob.bin',
    'snapshot_blob.bin',
  ],
  'linux': [
    PROJECT_NAME,  # 'electron'
    'content_shell.pak',
    'icudtl.dat',
    'libffmpeg.so',
    'libnode.so',
    'blink_image_resources_200_percent.pak',
    'content_resources_200_percent.pak',
    'ui_resources_200_percent.pak',
    'views_resources_200_percent.pak',
    'natives_blob.bin',
    'snapshot_blob.bin',
  ],
}
TARGET_DIRECTORIES = {
  'darwin': [
    '{0}.app'.format(PRODUCT_NAME),
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


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  force_build()
  create_symbols()
  copy_binaries()
  copy_chrome_binary('chromedriver')
  copy_chrome_binary('mksnapshot')
  copy_license()
  copy_api_json_schema()

  if PLATFORM == 'linux':
    strip_binaries()

  create_version()
  create_dist_zip()
  create_chrome_binary_zip('chromedriver', get_chromedriver_version())
  create_chrome_binary_zip('mksnapshot', ELECTRON_VERSION)
  create_ffmpeg_zip()
  create_symbols_zip()


def force_build():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  execute([sys.executable, build, '-c', 'Release'])


def copy_binaries():
  for binary in TARGET_BINARIES[PLATFORM]:
    shutil.copy2(os.path.join(OUT_DIR, binary), DIST_DIR)

  for directory in TARGET_DIRECTORIES[PLATFORM]:
    shutil.copytree(os.path.join(OUT_DIR, directory),
                    os.path.join(DIST_DIR, directory),
                    symlinks=True)


def copy_chrome_binary(binary):
  if PLATFORM == 'win32':
    binary += '.exe'
  src = os.path.join(CHROMIUM_DIR, binary)
  dest = os.path.join(DIST_DIR, binary)

  # Copy file and keep the executable bit.
  shutil.copyfile(src, dest)
  os.chmod(dest, os.stat(dest).st_mode | stat.S_IEXEC)


def copy_license():
  shutil.copy2(os.path.join(CHROMIUM_DIR, '..', 'LICENSES.chromium.html'),
               DIST_DIR)
  shutil.copy2(os.path.join(SOURCE_ROOT, 'LICENSE'), DIST_DIR)

def copy_api_json_schema():
  shutil.copy2(os.path.join(OUT_DIR, 'electron-api.json'), DIST_DIR)

def strip_binaries():
  for binary in TARGET_BINARIES[PLATFORM]:
    if binary.endswith('.so') or '.' not in binary:
      strip_binary(os.path.join(DIST_DIR, binary))


def strip_binary(binary_path):
    if get_target_arch() == 'arm':
      strip = 'arm-linux-gnueabihf-strip'
    else:
      strip = 'strip'
    execute([strip, binary_path])


def create_version():
  version_path = os.path.join(SOURCE_ROOT, 'dist', 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(ELECTRON_VERSION)


def create_symbols():
  destination = os.path.join(DIST_DIR, '{0}.breakpad.syms'.format(PROJECT_NAME))
  dump_symbols = os.path.join(SOURCE_ROOT, 'script', 'dump-symbols.py')
  execute([sys.executable, dump_symbols, destination])

  if PLATFORM == 'darwin':
    dsyms = glob.glob(os.path.join(OUT_DIR, '*.dSYM'))
    for dsym in dsyms:
      shutil.copytree(dsym, os.path.join(DIST_DIR, os.path.basename(dsym)))
  elif PLATFORM == 'win32':
    pdbs = glob.glob(os.path.join(OUT_DIR, '*.pdb'))
    for pdb in pdbs:
      shutil.copy2(pdb, DIST_DIR)


def create_dist_zip():
  dist_name = get_zip_name(PROJECT_NAME, ELECTRON_VERSION)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = TARGET_BINARIES[PLATFORM] +  ['LICENSE', 'LICENSES.chromium.html',
                                          'version']
    dirs = TARGET_DIRECTORIES[PLATFORM]
    make_zip(zip_file, files, dirs)


def create_chrome_binary_zip(binary, version):
  dist_name = get_zip_name(binary, version)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE', 'LICENSES.chromium.html']
    if PLATFORM == 'win32':
      files += [binary + '.exe']
    else:
      files += [binary]
    make_zip(zip_file, files, [])


def create_ffmpeg_zip():
  dist_name = get_zip_name('ffmpeg', ELECTRON_VERSION)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  if PLATFORM == 'darwin':
    ffmpeg_name = 'libffmpeg.dylib'
  elif PLATFORM == 'linux':
    ffmpeg_name = 'libffmpeg.so'
  elif PLATFORM == 'win32':
    ffmpeg_name = 'ffmpeg.dll'

  shutil.copy2(os.path.join(CHROMIUM_DIR, '..', 'ffmpeg', ffmpeg_name),
               DIST_DIR)

  if PLATFORM == 'linux':
    strip_binary(os.path.join(DIST_DIR, ffmpeg_name))

  with scoped_cwd(DIST_DIR):
    make_zip(zip_file, [ffmpeg_name, 'LICENSE', 'LICENSES.chromium.html'], [])


def create_symbols_zip():
  dist_name = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'symbols')
  zip_file = os.path.join(DIST_DIR, dist_name)
  licenses = ['LICENSE', 'LICENSES.chromium.html', 'version']

  with scoped_cwd(DIST_DIR):
    dirs = ['{0}.breakpad.syms'.format(PROJECT_NAME)]
    make_zip(zip_file, licenses, dirs)

  if PLATFORM == 'darwin':
    dsym_name = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'dsym')
    with scoped_cwd(DIST_DIR):
      dsyms = glob.glob('*.dSYM')
      make_zip(os.path.join(DIST_DIR, dsym_name), licenses, dsyms)
  elif PLATFORM == 'win32':
    pdb_name = get_zip_name(PROJECT_NAME, ELECTRON_VERSION, 'pdb')
    with scoped_cwd(DIST_DIR):
      pdbs = glob.glob('*.pdb')
      make_zip(os.path.join(DIST_DIR, pdb_name), pdbs + licenses, [])


if __name__ == '__main__':
  sys.exit(main())
