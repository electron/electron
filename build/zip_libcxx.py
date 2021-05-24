#!/usr/bin/env python
from __future__ import print_function
import os
import subprocess
import sys
import zipfile

def execute(argv):
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT)
    return output
  except subprocess.CalledProcessError as e:
    print(e.output)
    raise e

def get_object_files(base_path, archive_name):
  archive_file = os.path.join(base_path, archive_name)
  output = execute(['nm', '-g', archive_file]).decode('ascii')
  object_files = set()
  lines = output.split("\n")
  for line in lines:
    if line.startswith(base_path):
      object_file = line.split(":")[0]
      object_files.add(object_file)
    if line.startswith('nm: '):
      object_file = line.split(":")[1].lstrip()
      object_files.add(object_file)
  return list(object_files) + [archive_file]

def main(argv):
  dist_zip, = argv
  out_dir = os.path.dirname(dist_zip)
  base_path_libcxx = os.path.join(out_dir, 'obj/buildtools/third_party/libc++')
  base_path_libcxxabi = os.path.join(out_dir, 'obj/buildtools/third_party/libc++abi')
  object_files_libcxx = get_object_files(base_path_libcxx, 'libc++.a')
  object_files_libcxxabi = get_object_files(base_path_libcxxabi, 'libc++abi.a')
  with zipfile.ZipFile(
      dist_zip, 'w', zipfile.ZIP_DEFLATED, allowZip64=True
    ) as z:
      object_files_libcxx.sort()
      for object_file in object_files_libcxx:
        z.write(object_file, os.path.relpath(object_file, base_path_libcxx))
      for object_file in object_files_libcxxabi:
        z.write(object_file, os.path.relpath(object_file, base_path_libcxxabi))

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))