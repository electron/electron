#!/usr/bin/env python3

from __future__ import print_function
import json
import os
import sys
from urllib.request import Request, urlopen

sys.path.append(
  os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../.."))

from lib.config import s3_config
from lib.util import s3put, scoped_cwd, safe_mkdir, get_out_dir, ELECTRON_DIR

OUT_DIR     = get_out_dir()

BASE_URL = 'https://electron-metadumper.herokuapp.com/?version='

version = sys.argv[1]
authToken = os.getenv('META_DUMPER_AUTH_HEADER')

def is_json(myjson):
  try:
    json.loads(myjson)
  except ValueError:
    return False
  return True

def get_content(retry_count = 5):
  try:
    request = Request(
      BASE_URL + version,
      headers={"Authorization" : authToken}
    )

    proposed_content = urlopen(
      request
    ).read()

    if is_json(proposed_content):
      return proposed_content
    print("bad attempt")
    raise Exception("Failed to fetch valid JSON from the metadumper service")
  except Exception as e:
    if retry_count == 0:
      raise e
    return get_content(retry_count - 1)

def main():
  if not authToken or authToken == "":
    raise Exception("Please set META_DUMPER_AUTH_HEADER")
  # Upload the index.json.
  with scoped_cwd(ELECTRON_DIR):
    safe_mkdir(OUT_DIR)
    index_json = os.path.relpath(os.path.join(OUT_DIR, 'index.json'))

    new_content = get_content()

    with open(index_json, "w") as f:
      f.write(new_content)

    bucket, access_key, secret_key = s3_config()
    s3put(bucket, access_key, secret_key, OUT_DIR, 'atom-shell/dist',
          [index_json])


if __name__ == '__main__':
  sys.exit(main())
