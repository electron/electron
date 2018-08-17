#!/usr/bin/env python

import os
import sys
import urllib2

from lib.config import s3_config
from lib.util import s3put, scoped_cwd, safe_mkdir

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR     = os.path.join(SOURCE_ROOT, 'out', 'D')

BASE_URL = 'https://electron-metadumper.herokuapp.com/?version='

version = sys.argv[1]
authToken = os.getenv('META_DUMPER_AUTH_HEADER')

def main():
  if not authToken or authToken == "":
    raise Exception("Please set META_DUMPER_AUTH_HEADER")
  # Upload the index.json.
  with scoped_cwd(SOURCE_ROOT):
    safe_mkdir(OUT_DIR)
    index_json = os.path.relpath(os.path.join(OUT_DIR, 'index.json'))

    request = urllib2.Request(
      BASE_URL + version,
      headers={"Authorization" : authToken}
    )

    new_content = urllib2.urlopen(
      request
    ).read()

    with open(index_json, "w") as f:
      f.write(new_content)

    bucket, access_key, secret_key = s3_config()
    s3put(bucket, access_key, secret_key, OUT_DIR, 'atom-shell/dist',
          [index_json])


if __name__ == '__main__':
  sys.exit(main())
