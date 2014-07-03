#!/usr/bin/env python

import platform
import sys

NODE_VERSION = 'v0.11.13'
BASE_URL = 'https://gh-contractor-zcbenz.s3.amazonaws.com/libchromiumcontent'
LIBCHROMIUMCONTENT_COMMIT = '93f4be4bc30f98a445e4db2a2759057f0f01de01'

ARCH = {
    'cygwin': '32bit',
    'darwin': '64bit',
    'linux2': platform.architecture()[0],
    'win32': '32bit',
}[sys.platform]
DIST_ARCH = {
    '32bit': 'ia32',
    '64bit': 'x64',
}[ARCH]

TARGET_PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]
