#!/usr/bin/env python
from __future__ import print_function
import os
import subprocess
import sys
import zipfile

EXTENSIONS_TO_SKIP = [
  '.pdb',
  '.mojom.js',
  '.mojom-lite.js',
  '.info'
]

PATHS_TO_SKIP = [
  'angledata', #Skipping because it is an output of //ui/gl that we don't need
  './libVkICD_mock_', #Skipping because these are outputs that we don't need
  './VkICD_mock_', #Skipping because these are outputs that we don't need

  # Skipping because its an output of create_bundle from //build/config/mac/rules.gni
  # that we don't need
  'Electron.dSYM',

  # //chrome/browser:resources depends on this via
  # //chrome/browser/resources/ssl/ssl_error_assistant, but we don't need to
  # ship it.
  'pyproto',

  # On Windows, this binary doesn't exist (the crashpad handler is built-in).
  # On MacOS, the binary is called 'chrome_crashpad_handler' and is inside the
  # app bundle.
  # On Linux, we don't use crashpad, but this binary is still built for some
  # reason. Exclude it from the zip.
  './crashpad_handler',
]

def skip_path(dep, dist_zip, target_cpu):
  # Skip specific paths and extensions as well as the following special case:
  # snapshot_blob.bin is a dependency of mksnapshot.zip because
  # v8_context_generator needs it, but this file does not get generated for arm
  # and arm 64 binaries of mksnapshot since they are built on x64 hardware.
  # Consumers of arm and arm64 mksnapshot can generate snapshot_blob.bin
  # themselves by running mksnapshot.
  should_skip = (
    any(dep.startswith(path) for path in PATHS_TO_SKIP) or
    any(dep.endswith(ext) for ext in EXTENSIONS_TO_SKIP) or
    ('arm' in target_cpu and dist_zip == 'mksnapshot.zip' and dep == 'snapshot_blob.bin'))
  if should_skip:
    print("Skipping {}".format(dep))
  return should_skip

def execute(argv):
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT)
    return output
  except subprocess.CalledProcessError as e:
    print(e.output)
    raise e

def main(argv):
  dist_zip, runtime_deps, target_cpu, target_os, flatten_val = argv
  should_flatten = flatten_val == "true"
  dist_files = set()
  with open(runtime_deps) as f:
    for dep in f.readlines():
      dep = dep.strip()
      if not skip_path(dep, dist_zip, target_cpu):
        dist_files.add(dep)
  if sys.platform == 'darwin' and not should_flatten:
    execute(['zip', '-r', '-y', dist_zip] + list(dist_files))
  else:
    with zipfile.ZipFile(dist_zip, 'w', zipfile.ZIP_DEFLATED, allowZip64=True) as z:
      for dep in dist_files:
        if os.path.isdir(dep):
          for root, dirs, files in os.walk(dep):
            for file in files:
              z.write(os.path.join(root, file))
        else:
          basename = os.path.basename(dep)
          dirname = os.path.dirname(dep)
          arcname = os.path.join(dirname, 'chrome-sandbox') if basename == 'chrome_sandbox' else dep
          z.write(dep, os.path.basename(arcname) if should_flatten else arcname)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
