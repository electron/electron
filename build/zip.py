#!/usr/bin/env python
import os
import subprocess
import sys
import zipfile

LINUX_BINARIES_TO_STRIP = [
  'electron',
  'libffmpeg.so',
  'libnode.so'
]

def strip_binaries(target_cpu, dep):
  for binary in LINUX_BINARIES_TO_STRIP:
    if dep.endswith(binary):
     print 'stripping binary: ' + dep
     strip_binary(dep, target_cpu)

def strip_binary(binary_path, target_cpu):
  if target_cpu == 'arm':
    strip = 'arm-linux-gnueabihf-strip'
  elif target_cpu == 'arm64':
    strip = 'aarch64-linux-gnu-strip'
  elif target_cpu == 'mips64el':
    strip = 'mips64el-redhat-linux-strip'
  else:
    strip = 'strip'
  execute([strip, binary_path])

def execute(argv):
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT)
    return output
  except subprocess.CalledProcessError as e:
    print e.output
    raise e

def main(argv):
  dist_zip, runtime_deps, target_cpu, target_os = argv
  dist_files = []
  with open(runtime_deps) as f:
    for dep in f.readlines():
      dep = dep.strip()
      dist_files += [dep]
  print 'deps are'
  print dist_files
  if sys.platform == 'darwin':
    mac_zip_results = execute(['zip', '-r', '-y', dist_zip] + dist_files)
    print "done zipping results"
    print mac_zip_results
  else:
    with zipfile.ZipFile(dist_zip, 'w', zipfile.ZIP_DEFLATED) as z:
      for dep in dist_files:
        if target_os == 'linux':
            strip_binaries(target_cpu, dep)
        if os.path.isdir(dep):
          for root, dirs, files in os.walk(dep):
            for file in files:
              z.write(os.path.join(root, file))
        else:
          z.write(dep)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
