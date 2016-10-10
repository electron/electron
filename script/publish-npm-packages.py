#!/usr/bin/env python
 
import glob
import os
import re
import shutil
import subprocess
import sys
import stat
import urllib
import tarfile
import json
 
from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, PLATFORM, \
                        get_target_arch, get_chromedriver_version, \
                        get_zip_name
from lib.util import scoped_cwd, rm_rf, get_electron_version, make_zip, \
                      execute, electron_gyp
 
 
ELECTRON_VERSION = get_electron_version()[1:]
TARGET_ARCH = get_target_arch()
BAMS_URL = 'https://bams-sami.int.thomsonreuters.com/artifactory/default.npm.local/UniversalContainer'
SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
ELECTRON_NPM_DIR = os.path.join(SOURCE_ROOT, 'npm')
 
 
def main():
  create_and_publish_package('tr-electron-download', False)
  create_and_publish_package('tr-electron', True)
      

def create_and_publish_package(name, updateVersion):
  
  package_path = os.path.join(ELECTRON_NPM_DIR, name)
  
  # Read current version
  version = "N/A"
  with open(os.path.join(package_path, 'package.json'), 'r') as json_file:
    json_obj = json.load(json_file)
  
  if (updateVersion == True):
    
    json_obj['version'] = ELECTRON_VERSION
  
    with open(os.path.join(package_path, 'package.json'), 'w') as json_file:
      json.dump(json_obj, json_file, indent=4, encoding="utf-8", default=None, sort_keys=False)
  
  version = json_obj['version']
  
  # Delete existing file
  fileName = name + '-' + version + '.tgz'
  localPackagePath = os.path.join(ELECTRON_NPM_DIR, name, fileName)  
  
  # Create the NPM package  
  os.chdir(package_path)
  os.system('npm pack')
  
  url = BAMS_URL + '/' + name + '/' + fileName
  
  # Delete any exisiting similar version from BAMS
  cmd = os.path.join(ELECTRON_NPM_DIR, 'tools', 'curl.exe') + ' -u "' + sys.argv[1] + '" -X DELETE ' + url
  os.system(cmd)
  
  # Put the new version to BAMS
  cmd = os.path.join(ELECTRON_NPM_DIR, 'tools', 'curl.exe') + ' -u "' + sys.argv[1] + '" -X PUT ' + url + ' -T ' + localPackagePath
  os.system(cmd)

  
if __name__ == '__main__':
  sys.exit(main())
