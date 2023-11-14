import sys
import os
import subprocess

def main(argv):
  os.chdir(argv[1])
  subprocess.Popen(argv[2:])

if __name__ == '__main__':
  main(sys.argv)
