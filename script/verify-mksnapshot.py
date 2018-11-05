#!/usr/bin/env python

import glob
import os
import shutil
import subprocess
import sys

from lib.util import electron_gyp, rm_rf, scoped_cwd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SNAPSHOT_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'download',
                                 'libchromiumcontent', 'static_library')
PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']
SNAPSHOT_SOURCE = os.path.join(SOURCE_ROOT, 'spec', 'fixtures', 'testsnap.js')

def main():
  os.chdir(SOURCE_ROOT)
  app_path = create_app_copy()
  blob_out_path = app_path
  snapshot_gen_path =  os.path.join(SNAPSHOT_DIR, 'snapshot_gen', '*')
  snapshot_gen_files = glob.glob(snapshot_gen_path)
  if sys.platform == 'darwin':
    electron = os.path.join(app_path, 'Contents', 'MacOS', PRODUCT_NAME)
    blob_out_path = os.path.join(app_path, 'Contents', 'Frameworks',
                    '{0} Framework.framework'.format(PROJECT_NAME),
                    'Resources')
    snapshot_gen_files += [ os.path.join(SNAPSHOT_DIR, 'libffmpeg.dylib') ]
  elif sys.platform == 'win32':
    electron = os.path.join(app_path, '{0}.exe'.format(PROJECT_NAME))
    snapshot_gen_files += [
      os.path.join(SNAPSHOT_DIR, 'ffmpeg.dll'),
      os.path.join(SNAPSHOT_DIR, 'ffmpeg.dll.lib'),
    ]
  else:
    electron = os.path.join(app_path, PROJECT_NAME)
    snapshot_gen_files += [ os.path.join(SNAPSHOT_DIR, 'libffmpeg.so') ]

  # Copy mksnapshot and friends to the directory the snapshot_blob should be
  # generated in.
  mksnapshot_binary = get_binary_path('mksnapshot', SNAPSHOT_DIR)
  shutil.copy2(mksnapshot_binary, blob_out_path)
  for gen_file in snapshot_gen_files:
    shutil.copy2(gen_file, blob_out_path)

  returncode = 0
  try:
    with scoped_cwd(blob_out_path):
      mkargs = [ get_binary_path('mksnapshot', blob_out_path), \
                 SNAPSHOT_SOURCE, '--startup_blob', 'snapshot_blob.bin' ]
      subprocess.check_call(mkargs)
      print 'ok mksnapshot successfully created snapshot_blob.bin.'
      context_snapshot = 'v8_context_snapshot.bin'
      context_snapshot_path = os.path.join(blob_out_path, context_snapshot)
      gen_binary = get_binary_path('v8_context_snapshot_generator', \
                                   blob_out_path)
      genargs = [ gen_binary, \
                 '--output_file={0}'.format(context_snapshot_path) ]
      subprocess.check_call(genargs)
      print 'ok v8_context_snapshot_generator successfully created ' \
            + context_snapshot

    test_path = os.path.join('spec', 'fixtures', 'snapshot-items-available.js')
    subprocess.check_call([electron, test_path])
    print 'ok successfully used custom snapshot.'
  except subprocess.CalledProcessError as e:
    print 'not ok an error was encountered while testing mksnapshot.'
    print e
    returncode = e.returncode
  except KeyboardInterrupt:
    print 'Other error'
    returncode = 0

  return returncode


# Create copy of app to create custom snapshot
def create_app_copy():
  initial_app_path = os.path.join(SOURCE_ROOT, 'out', 'R')
  app_path = os.path.join(SOURCE_ROOT, 'out', 'R-mksnapshot-test')

  if sys.platform == 'darwin':
    app_name = '{0}.app'.format(PRODUCT_NAME)
    initial_app_path = os.path.join(initial_app_path, app_name)
    app_path = os.path.join(app_path, app_name)

  rm_rf(app_path)
  shutil.copytree(initial_app_path, app_path, symlinks=True)
  return app_path

def get_binary_path(binary_name, root_path):
  if sys.platform == 'win32':
    binary_path = os.path.join(root_path, '{0}.exe'.format(binary_name))
  else:
    binary_path = os.path.join(root_path, binary_name)
  return binary_path

if __name__ == '__main__':
  sys.exit(main())
