#!/usr/bin/env python

import json
import os
import ssl
import subprocess
import sys
import urllib2

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

def check_tls(verbose):
  process = subprocess.Popen(
    'node tls.js',
    cwd=os.path.dirname(os.path.realpath(__file__)),
    shell=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT
  )

  port = process.stdout.readline()
  localhost_url = 'https://localhost:' + port

  response = json.load(urllib2.urlopen(localhost_url, context=ctx))
  tls = response['protocol']
  process.wait()

  if sys.platform == "linux" or sys.platform == "linux2":
    tutorial = "./docs/development/build-instructions-linux.md"
  elif sys.platform == "darwin":
    tutorial = "./docs/development/build-instructions-osx.md"
  elif sys.platform == "win32":
    tutorial = "./docs/development/build-instructions-windows.md"
  else:
    tutorial = "build instructions for your operating system" \
      + "in ./docs/development/"

  if tls == "TLSv1" or tls == "TLSv1.1":
    print "Your system/python combination is using an outdated security" \
      + "protocol and will not be able to compile Electron. Please see " \
      + tutorial + "." \
      + "for instructions on how to update Python."
    sys.exit(1)
  else:
    if verbose:
      print "Your Python is using " + tls + ", which is sufficient for " \
        + "building Electron."

if __name__ == '__main__':
  check_tls(True)
  sys.exit(0)
