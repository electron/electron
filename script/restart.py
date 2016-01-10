#!/usr/bin/env python

import os
import platform
import subprocess
import sys
import time

def main():
  pid         = int(sys.argv[1])
  timeout     = int(sys.argv[2])
  system_os   = platform.system()
  startup_cmd = [os.path.abspath(arg) for arg in sys.argv[3:]]

  if system_os == 'Darwin' or system_os == 'Linux':
    if wait_for_exit_unix(pid, timeout):
      print(startup_cmd)
      subprocess.Popen(startup_cmd, close_fds=True)
      return 0
    else:
      print('Timeout exceeded.')
      return 1
  elif system_os == 'Windows':
    pass
  else:
    return 1

def wait_for_exit_unix(pid, timeout):
  wait_time = timeout
  while(wait_time > 0):
    try:
      os.kill(pid, 0)
    except OSError:
      return True
    time.sleep(1)
    wait_time -= 1000

  return False

def wait_for_exit_windows(timeout):
  wait_time = timeout
  while(wait_time > 0):
    time.sleep(1)
    wait_time -= 1000

  return False

if __name__ == '__main__':
  sys.exit(main())
