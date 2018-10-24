#!/usr/bin/env python

from lib import git
from lib.patches import patch_from_dir


patch_dirs = {
  'src/electron/patches/common/chromium':
    'src',

  'src/electron/patches/common/boringssl':
    'src/third_party/boringssl/src',

  'src/electron/patches/common/ffmpeg':
    'src/third_party/ffmpeg',

  'src/electron/patches/common/skia':
    'src/third_party/skia',

  'src/electron/patches/common/v8':
    'src/v8',
}


def apply_patches(dirs):
  for patch_dir, repo in dirs.iteritems():
    git.am(repo=repo, patch_data=patch_from_dir(patch_dir),
      committer_name="Electron Scripts", committer_email="scripts@electron")


def main():
  apply_patches(patch_dirs)


if __name__ == '__main__':
  main()
