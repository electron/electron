#!/usr/bin/env python3
"""Downloads Electron PGO profiles named by the build/pgo_profiles state files.

Each target has a <target>.pgo.txt state file naming the profile to download
(generated and uploaded by .github/workflows/pgo-generation.yml).

C++ profiles are saved under their full versioned names - the same name the
state file contains - because the build resolves the profile path by reading
the state file (see
patches/chromium/build_resolve_pgo_profiles_from_electron_state_files.patch).
Stale versions of a target's profile are removed after a successful download.

The V8 builtins profile is saved under the fixed name
electron-v8-builtins.profile, which release.gn references statically (GN args
files cannot read state files).

Run by gclient hooks (see electron's DEPS) and safe to run manually:

  python3 script/pgo/download-profiles.py --targets linux-x64,v8-builtins
"""

import argparse
import os
import re
import sys
import time
import urllib.error
import urllib.request

CDN_BASE_URL = os.environ.get(
    'ELECTRON_PGO_CDN_URL', 'https://dev-cdn-experimental.electronjs.org/pgo/'
)

PROFILES_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), '..', '..', 'build', 'pgo_profiles'
)

DOWNLOAD_RETRIES = 3
DOWNLOAD_RETRY_DELAY_S = 5

# Profile names have a strict format (electron-<target>-<timestamp>-<sha> plus
# an extension); reject anything else so a state file can never alter the
# download URL beyond selecting a file under the CDN prefix.
SAFE_PROFILE_NAME = re.compile(r'^[A-Za-z0-9._-]+$')

V8_BUILTINS_TARGET = 'v8-builtins'
V8_BUILTINS_LOCAL_NAME = 'electron-v8-builtins.profile'


def read_state_file(target):
    state_path = os.path.join(PROFILES_DIR, f'{target}.pgo.txt')
    if not os.path.isfile(state_path):
        raise SystemExit(
            f'error: no state file for target "{target}" at {state_path}'
        )
    with open(state_path, encoding='utf-8') as f:
        profile_name = f.read().strip()
    if (
        not profile_name
        or not SAFE_PROFILE_NAME.match(profile_name)
        # The regex bans path separators, so the only traversal-capable names
        # are the dot directories themselves.
        or profile_name in ('.', '..')
    ):
        raise SystemExit(
            f'error: invalid profile name {profile_name!r} in {state_path}'
        )
    return profile_name


def remove_stale_profiles(target, keep_name):
    """Removes old versions of a target's profile after an update."""
    prefix = f'electron-{target}-'
    for name in os.listdir(PROFILES_DIR):
        if (
            name.startswith(prefix)
            and name.endswith('.profdata')
            and name != keep_name
        ):
            os.remove(os.path.join(PROFILES_DIR, name))
            print(f'{target}: removed stale {name}')


def fetch(url, dest):
    for attempt in range(1, DOWNLOAD_RETRIES + 1):
        try:
            tmp_dest = dest + '.tmp'
            with urllib.request.urlopen(url, timeout=300) as response, open(
                tmp_dest, 'wb'
            ) as out:
                while True:
                    chunk = response.read(1024 * 1024)
                    if not chunk:
                        break
                    out.write(chunk)
            os.replace(tmp_dest, dest)
            return
        except (urllib.error.URLError, OSError) as e:
            if os.path.isfile(dest + '.tmp'):
                os.remove(dest + '.tmp')
            if attempt == DOWNLOAD_RETRIES:
                raise SystemExit(f'error: failed to download {url}: {e}') from e
            print(f'attempt {attempt} failed ({e}), retrying')
            time.sleep(DOWNLOAD_RETRY_DELAY_S * attempt)


def download_cpp_profile(target):
    cdn_name = read_state_file(target)
    dest = os.path.join(PROFILES_DIR, cdn_name)

    # Versioned names mean presence implies correctness.
    if os.path.isfile(dest):
        print(f'{target}: {cdn_name} already present')
        return

    url = CDN_BASE_URL + cdn_name
    print(f'{target}: downloading {url}')
    fetch(url, dest)
    remove_stale_profiles(target, cdn_name)
    size_mb = os.path.getsize(dest) / (1024 * 1024)
    print(f'{target}: wrote {cdn_name} ({size_mb:.1f} MB)')


def download_v8_builtins_profile():
    cdn_name = read_state_file(V8_BUILTINS_TARGET)
    dest = os.path.join(PROFILES_DIR, V8_BUILTINS_LOCAL_NAME)
    version_marker = dest + '.version'

    # Fixed local name: a version marker tracks which CDN profile it holds.
    if os.path.isfile(dest) and os.path.isfile(version_marker):
        with open(version_marker, encoding='utf-8') as f:
            if f.read().strip() == cdn_name:
                print(f'{V8_BUILTINS_TARGET}: {cdn_name} already present')
                return

    url = CDN_BASE_URL + cdn_name
    print(f'{V8_BUILTINS_TARGET}: downloading {url}')
    fetch(url, dest)
    with open(version_marker, 'w', encoding='utf-8') as f:
        f.write(cdn_name + '\n')
    size_mb = os.path.getsize(dest) / (1024 * 1024)
    print(
        f'{V8_BUILTINS_TARGET}: wrote {V8_BUILTINS_LOCAL_NAME} '
        f'({size_mb:.1f} MB, {cdn_name})'
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--targets',
        required=True,
        help='comma-separated profile targets, e.g. linux-x64,linux-arm,v8-builtins',
    )
    args = parser.parse_args()

    os.makedirs(PROFILES_DIR, exist_ok=True)
    for target in args.targets.split(','):
        target = target.strip()
        if target == V8_BUILTINS_TARGET:
            download_v8_builtins_profile()
        else:
            download_cpp_profile(target)
    return 0


if __name__ == '__main__':
    sys.exit(main())
