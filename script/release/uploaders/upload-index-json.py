#!/usr/bin/env python3

import json
import os
import sys
from urllib.request import Request, urlopen

sys.path.append(
  os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../.."))

from lib.util import store_artifact, scoped_cwd, safe_mkdir, get_out_dir, \
  ELECTRON_DIR

OUT_DIR     = get_out_dir()

META_DUMPER_URL = os.environ.get('META_DUMPER_URL')
BASE_URL = (META_DUMPER_URL or 'https://electron-metadumper.herokuapp.com/') + '?version='

AUTH_TOKEN = os.getenv('META_DUMPER_AUTH_HEADER')

def is_json(myjson):
  try:
    json.loads(myjson)
  except ValueError:
    return False
  return True

def get_content(version, retry_count=5):
  for attempt in range(retry_count):
    try:
      headers = {} if META_DUMPER_URL else {"Authorization": AUTH_TOKEN}
      request = Request(BASE_URL + version, headers=headers)

      with urlopen(request) as resp:
        proposed_content = resp.read()

      if is_json(proposed_content):
        return proposed_content

      print("Received content is not valid JSON.")
      if attempt == retry_count - 1:
        return None

    except Exception as e:
      print(f"Attempt {attempt + 1} failed: {e}")
      if attempt == retry_count - 1:
        return None

    if attempt < retry_count - 1:
      print("Retrying...")

  return None

def main():
  if not META_DUMPER_URL and not AUTH_TOKEN:
    raise Exception("Please set META_DUMPER_AUTH_HEADER")

  if len(sys.argv) < 2 or not sys.argv[1]:
    raise Exception("Version is required")

  version = sys.argv[1]

  with scoped_cwd(ELECTRON_DIR):
    safe_mkdir(OUT_DIR)
    index_json = os.path.relpath(os.path.join(OUT_DIR, 'index.json'))
    new_content = get_content(version)

    if new_content is None:
      raise Exception("Failed to fetch valid JSON after maximum retries.")

    with open(index_json, "wb") as f:
      f.write(new_content)

    store_artifact(OUT_DIR, 'headers/dist', [index_json])

if __name__ == '__main__':
  sys.exit(main())
