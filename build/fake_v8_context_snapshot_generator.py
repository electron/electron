import os
import shutil
import sys

if os.path.exists(sys.argv[2]):
  os.remove(sys.argv[2])

shutil.copy(sys.argv[1], sys.argv[2])
