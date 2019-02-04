import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))

def main():
  # Proxy all args to gn-asar.js node script
  script = os.path.join(SOURCE_ROOT, 'script', 'gn-asar.js')
  subprocess.check_call(['node', script] + [str(x) for x in sys.argv[1:]])

if __name__ == '__main__':
  sys.exit(main())
