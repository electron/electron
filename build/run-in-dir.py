import sys
import os
import subprocess

def main(argv):
  os.chdir(argv[1])
  p = subprocess.Popen(argv[2:])
  return p.wait()

if __name__ == '__main__':
  sys.exit(main(sys.argv))
