import sys
import os
import subprocess

def main(argv):
  src = os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
  ninja_path = os.path.join(src, 'third_party', 'depot_tools', 'ninja')
  os.execv(ninja_path, argv)

if __name__ == '__main__':
  main(sys.argv)
