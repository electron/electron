import os
import subprocess
import sys

# Helper to run gn format on multiple files
# (gn only formats a single file at a time)
def main():
  for gn_file in sys.argv[1:]:
    subprocess.check_call(['gn', 'format', gn_file])

if __name__ == '__main__':
  sys.exit(main())
