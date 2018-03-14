#!/usr/bin/env python

import json
import urllib2
import sys

def main():
  tls = json.load(urllib2.urlopen('https://www.howsmyssl.com/a/check'))['tls_version']

  if sys.platform == "linux" or sys.platform == "linux2":
    tutorial = "./docs/development/build-instructions-linux.md"
  elif sys.platform == "darwin":
    tutorial = "./docs/development/build-instructions-osx.md"
  elif sys.platform == "win32":
    tutorial = "./docs/development/build-instructions-windows.md"
  else:
    tutorial = "build instructions for your operation system in ./docs/development/"

  if tls == "TLS 1.2":
    print "Your system/python combination is using an outdated security protocol and will not"
    print "be able to compile Electron. Please see " + tutorial + "."
    print "for instructions on how to update Python."
    sys.exit(1)
  else:
    print "Your Python is using " + tls + ", which is sufficient for building Electron."
    sys.exit(0)

if __name__ == '__main__':
  main()
