#!/usr/bin/env python
import os
import sys
import zipfile

def main(argv):
  dist_zip, runtime_deps = argv
  with zipfile.ZipFile(dist_zip, 'w', allowZip64=True) as z:
    with open(runtime_deps) as f:
      for dep in f.readlines():
        dep = dep.strip()
        if os.path.isdir(dep):
          for root, dirs, files in os.walk(dep):
            for file in files:
              z.write(os.path.join(root, file))
        else:
          z.write(dep)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
