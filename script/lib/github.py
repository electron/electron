#!/usr/bin/env python

import json
import os
import re
import sys

REQUESTS_DIR = os.path.abspath(os.path.join(__file__, '..', '..', '..',
                                            'vendor', 'requests'))
sys.path.append(os.path.join(REQUESTS_DIR, 'build', 'lib'))
sys.path.append(os.path.join(REQUESTS_DIR, 'build', 'lib.linux-x86_64-2.7'))
import requests

GITHUB_URL = 'https://api.github.com'
GITHUB_UPLOAD_ASSET_URL = 'https://uploads.github.com'

class GitHub:
  def __init__(self, access_token):
    self._authorization = 'token %s' % access_token

    pattern = '^/repos/{0}/{0}/releases/{1}/assets$'.format('[^/]+', '[0-9]+')
    self._releases_upload_api_pattern = re.compile(pattern)

  def __getattr__(self, attr):
    return _Callable(self, '/%s' % attr)

  def send(self, method, path, **kw):
    if not 'headers' in kw:
      kw['headers'] = dict()
    headers = kw['headers']
    headers['Authorization'] = self._authorization
    headers['Accept'] = 'application/vnd.github.manifold-preview'

    # Switch to a different domain for the releases uploading API.
    if self._releases_upload_api_pattern.match(path):
      url = '%s%s' % (GITHUB_UPLOAD_ASSET_URL, path)
    else:
      url = '%s%s' % (GITHUB_URL, path)
      # Data are sent in JSON format.
      if 'data' in kw:
        kw['data'] = json.dumps(kw['data'])

    r = getattr(requests, method)(url, **kw).json()
    if 'message' in r:
      raise Exception(json.dumps(r, indent=2, separators=(',', ': ')))
    return r


class _Executable:
  def __init__(self, gh, method, path):
    self._gh = gh
    self._method = method
    self._path = path

  def __call__(self, **kw):
    return self._gh.send(self._method, self._path, **kw)


class _Callable(object):
  def __init__(self, gh, name):
    self._gh = gh
    self._name = name

  def __call__(self, *args):
    if len(args) == 0:
      return self

    name = '%s/%s' % (self._name, '/'.join([str(arg) for arg in args]))
    return _Callable(self._gh, name)

  def __getattr__(self, attr):
    if attr in ['get', 'put', 'post', 'patch', 'delete']:
      return _Executable(self._gh, attr, self._name)

    name = '%s/%s' % (self._name, attr)
    return _Callable(self._gh, name)
