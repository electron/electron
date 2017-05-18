#!/usr/bin/env python

import os
import shutil
import subprocess
import sys

from lib.util import electron_gyp, rm_rf


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
FFMPEG_LIBCC_PATH = os.path.join(SOURCE_ROOT, 'vendor', 'download',
                                 'libchromiumcontent', 'ffmpeg')

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']


def main():
  os.chdir(SOURCE_ROOT)

  if len(sys.argv) == 2 and sys.argv[1] == '-R':
    config = 'R'
  else:
    config = 'D'

  app_path = create_app_copy(config)

  if sys.platform == 'darwin':
    electron = os.path.join(app_path, 'Contents', 'MacOS', PRODUCT_NAME)
    ffmpeg_name = 'libffmpeg.dylib'
    ffmpeg_app_path = os.path.join(app_path, 'Contents', 'Frameworks',
                    '{0} Framework.framework'.format(PROJECT_NAME),
                    'Libraries')
  elif sys.platform == 'win32':
    electron = os.path.join(app_path, '{0}.exe'.format(PROJECT_NAME))
    ffmpeg_app_path = app_path
    ffmpeg_name = 'ffmpeg.dll'
  else:
    electron = os.path.join(app_path, PROJECT_NAME)
    ffmpeg_app_path = app_path
    ffmpeg_name = 'libffmpeg.so'

  # Copy ffmpeg without proprietary codecs into app
  shutil.copy(os.path.join(FFMPEG_LIBCC_PATH, ffmpeg_name), ffmpeg_app_path)

  returncode = 0
  try:
    test_path = os.path.join('spec', 'fixtures', 'no-proprietary-codecs.js')
    subprocess.check_call([electron, test_path] + sys.argv[1:])
  except subprocess.CalledProcessError as e:
    returncode = e.returncode
  except KeyboardInterrupt:
    returncode = 0

  return returncode


# Create copy of app to install ffmpeg library without proprietary codecs into
def create_app_copy(config):
  initial_app_path = os.path.join(SOURCE_ROOT, 'out', config)
  app_path = os.path.join(SOURCE_ROOT, 'out', config + '-no-proprietary-codecs')

  if sys.platform == 'darwin':
    app_name = '{0}.app'.format(PRODUCT_NAME)
    initial_app_path = os.path.join(initial_app_path, app_name)
    app_path = os.path.join(app_path, app_name)

  rm_rf(app_path)
  shutil.copytree(initial_app_path, app_path, symlinks=True)
  return app_path


if __name__ == '__main__':
  sys.exit(main())
