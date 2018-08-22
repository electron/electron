#!/usr/bin/env python
import argparse
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
  args = parse_args()
  os.chdir(args.source_root)

  app_path = create_app_copy(args)

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
  shutil.copy(os.path.join(args.ffmpeg_path, ffmpeg_name), ffmpeg_app_path)

  returncode = 0
  try:
    test_path = os.path.join(SOURCE_ROOT, 'spec', 'fixtures',
        'no-proprietary-codecs.js')
    subprocess.check_call([electron, test_path] + sys.argv[1:])
  except subprocess.CalledProcessError as e:
    returncode = e.returncode
  except KeyboardInterrupt:
    returncode = 0

  return returncode


# Create copy of app to install ffmpeg library without proprietary codecs into
def create_app_copy(args):
  initial_app_path = os.path.join(args.source_root, 'out', args.config)
  app_path = os.path.join(args.source_root, 'out',
    args.config + '-no-proprietary-codecs')

  if sys.platform == 'darwin':
    app_name = '{0}.app'.format(PRODUCT_NAME)
    initial_app_path = os.path.join(initial_app_path, app_name)
    app_path = os.path.join(app_path, app_name)

  rm_rf(app_path)
  shutil.copytree(initial_app_path, app_path, symlinks=True)
  return app_path

def parse_args():
  parser = argparse.ArgumentParser(description='Test non-proprietary ffmpeg')
  parser.add_argument('-c', '--config',
                      help='Test with Release or Debug configuration',
                      default='D',
                      required=False)
  parser.add_argument('--source-root',
                      default=SOURCE_ROOT,
                      required=False)
  parser.add_argument('--ffmpeg-path',
                      default=FFMPEG_LIBCC_PATH,
                      required=False)
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
