#!/usr/bin/env python

import zipfile
import sys

def main(zip_path, manifest_out):
  with open(manifest_out, 'w') as manifest, \
      zipfile.ZipFile(zip_path, 'r', allowZip64=True) as z:
    for name in sorted(z.namelist()):
      manifest.write(name + '\n')
  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1], sys.argv[2]))
