#!/usr/bin/env python
import argparse
import os
import shutil
import subprocess
import sys

from lib.gn import gn
from lib.util import rm_rf


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  args = parse_args()

  initial_app_path = os.path.join(os.path.abspath(args.source_root), args.build_dir)
  app_path = create_app_copy(initial_app_path)

  # Those are the same in the original app and its copy.
  # So it is ok to retrieve them from the original build dir and use in the copy.
  product_name = gn(initial_app_path).args().get_string('electron_product_name')
  project_name = gn(initial_app_path).args().get_string('electron_project_name')

  if sys.platform == 'darwin':
    electron = os.path.join(app_path, 'Contents', 'MacOS', product_name)
    ffmpeg_name = 'libffmpeg.dylib'
    ffmpeg_app_path = os.path.join(app_path, 'Contents', 'Frameworks',
                    '{0} Framework.framework'.format(product_name),
                    'Libraries')
  elif sys.platform == 'win32':
    electron = os.path.join(app_path, '{0}.exe'.format(project_name))
    ffmpeg_app_path = app_path
    ffmpeg_name = 'ffmpeg.dll'
  else:
    electron = os.path.join(app_path, project_name)
    ffmpeg_app_path = app_path
    ffmpeg_name = 'libffmpeg.so'

  # Copy ffmpeg without proprietary codecs into app.
  ffmpeg_lib_path = os.path.join(os.path.abspath(args.source_root), args.ffmpeg_path, ffmpeg_name)
  shutil.copy(ffmpeg_lib_path, ffmpeg_app_path)

  returncode = 0
  try:
    test_path = os.path.join(SOURCE_ROOT, 'spec', 'fixtures',
        'no-proprietary-codecs.js')
    subprocess.check_call([electron, test_path] + sys.argv[1:])
  except subprocess.CalledProcessError as e:
    returncode = e.returncode
  except KeyboardInterrupt:
    returncode = 0

  if returncode == 0:
    print 'ok Non proprietary ffmpeg does not contain proprietary codes.'
  return returncode


# Create copy of app to install ffmpeg library without proprietary codecs into
def create_app_copy(initial_app_path):
  app_path = os.path.join(os.path.dirname(initial_app_path),
                          os.path.basename(initial_app_path) + '-no-proprietary-codecs')

  if sys.platform == 'darwin':
    product_name = gn(initial_app_path).args().get_string('electron_product_name')
    app_name = '{0}.app'.format(product_name)
    initial_app_path = os.path.join(initial_app_path, app_name)
    app_path = os.path.join(app_path, app_name)

  rm_rf(app_path)
  shutil.copytree(initial_app_path, app_path, symlinks=True)
  return app_path

def parse_args():
  parser = argparse.ArgumentParser(description='Test non-proprietary ffmpeg')
  parser.add_argument('-b', '--build-dir',
                      help='Path to an Electron build folder. Relative to the --source-root.',
                      default=None,
                      required=True)
  parser.add_argument('--source-root',
                      default=SOURCE_ROOT,
                      required=False)
  parser.add_argument('--ffmpeg-path',
                      help='Path to a folder with a ffmpeg lib. Relative to the --source-root.',
                      default=None,
                      required=True)
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
