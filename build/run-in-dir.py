import sys
import os

def main(argv):
  cwd = argv[1]
  os.chdir(cwd)
  os.execv(sys.executable, [sys.executable] + argv[2:])

if __name__ == '__main__':
  main(sys.argv)
