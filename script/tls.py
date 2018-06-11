#!/usr/bin/env python

import json
import urllib2
import sys

def check_tls(verbose):
  response = json.load(urllib2.urlopen('https://www.howsmyssl.com/a/check'))
  tls = response['tls_version']

  if sys.platform == "linux" or sys.platform == "linux2":
    tutorial = "./docs/development/build-instructions-linux.md"
  elif sys.platform == "darwin":
    tutorial = "./docs/development/build-instructions-osx.md"
  elif sys.platform == "win32":
    tutorial = "./docs/development/build-instructions-windows.md"
  else:
    tutorial = "build instructions for your operating system" \
      + "in ./docs/development/"

  if tls == "TLS 1.0":
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
