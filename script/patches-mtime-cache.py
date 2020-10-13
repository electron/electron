#!/usr/bin/env python

from __future__ import print_function

import argparse
import hashlib
import json
import os
import posixpath
import sys

from lib.patches import patch_from_dir


def generate_cache(patches_config):
    mtime_cache = {}

    for patch_dir, repo in patches_config.items():
        for line in patch_from_dir(patch_dir).split("\n"):
            if line.startswith("+++"):
                file_path = posixpath.join(repo, line[6:])

                if not os.path.exists(file_path):
                    print("Skipping non-existent file:", file_path)
                    continue

                with open(file_path, "rb") as f:
                    mtime_cache[file_path] = {
                        "sha256": hashlib.sha256(f.read()).hexdigest(),
                        "atime": os.path.getatime(file_path),
                        "mtime": os.path.getmtime(file_path),
                    }

    return mtime_cache


def apply_mtimes(mtime_cache):
    for file_path, metadata in mtime_cache.items():
        if not os.path.exists(file_path):
            print("Skipping non-existent file:", file_path)
            continue

        with open(file_path, "rb") as f:
            if hashlib.sha256(f.read()).hexdigest() != metadata["sha256"]:
                continue

        os.utime(file_path, (metadata["atime"], metadata["mtime"]))


def main():
    parser = argparse.ArgumentParser(description="Make mtime cache for patched files")
    parser.add_argument(
        "--patches-config",
        type=argparse.FileType("r"),
        help="patches' config in the JSON format",
    )
    parser.add_argument("--cache-file", required=True, help="mtime cache file")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--generate", action="store_true", help="generate the cache")
    group.add_argument(
        "--apply", action="store_true", help="apply the mtimes from the cache"
    )

    args = parser.parse_args()

    if args.generate:
        if not args.patches_config:
            print("ERROR: --patches-config required for generating")
            sys.exit(1)

        with open(args.cache_file, mode="w") as f:
            mtime_cache = generate_cache(json.load(args.patches_config))
            json.dump(mtime_cache, f)
    elif args.apply:
        if not os.path.exists(args.cache_file):
            print("ERROR: --cache-file does not exist")
            sys.exit(1)

        with open(args.cache_file, mode="r") as f:
            apply_mtimes(json.load(f))


if __name__ == "__main__":
    main()
