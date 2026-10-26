#!/usr/bin/env python3
"""Downloads Electron PGO profiles named by the build/pgo_profiles state files.

Each target has a <target>.pgo.txt state file naming the profile to download
(generated and uploaded by .github/workflows/pgo-generation.yml). State files
contain a single line, either

  <profile-name> <sha256>

or the legacy hash-less form

  <profile-name>

When a sha256 is present, downloads (and previously downloaded local copies)
are verified against it and a mismatch is a hard failure - the CDN blobs are
mutable, so the checked-in state file is what pins the exact profile bytes a
build uses. Hash-less state files are still accepted for release branches
that predate hashes; they get a hash on their next profile update.

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
import hashlib
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

SHA256_HEX = re.compile(r'^[0-9a-f]{64}$')

V8_BUILTINS_TARGET = 'v8-builtins'
V8_BUILTINS_LOCAL_NAME = 'electron-v8-builtins.profile'


def read_state_file(target):
    """Returns (profile_name, expected_sha256), sha256 None for legacy files."""
    state_path = os.path.join(PROFILES_DIR, f'{target}.pgo.txt')
    if not os.path.isfile(state_path):
        raise SystemExit(
            f'error: no state file for target "{target}" at {state_path}'
        )
    with open(state_path, encoding='utf-8') as f:
        fields = f.read().split()
    profile_name = fields[0] if fields else ''
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
    expected_sha256 = None
    if len(fields) == 2:
        expected_sha256 = fields[1].lower()
        if not SHA256_HEX.match(expected_sha256):
            raise SystemExit(
                f'error: invalid sha256 {fields[1]!r} in {state_path}'
            )
    elif len(fields) > 2:
        raise SystemExit(
            f'error: malformed state file {state_path}: expected '
            f'"<name>" or "<name> <sha256>", got {len(fields)} fields'
        )
    return profile_name, expected_sha256


def file_sha256(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


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


def fetch(url, dest, expected_sha256=None):
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
            # Verify before the file can land at its final path. A mismatch is
            # not retried: the bytes arrived intact but are not the bytes the
            # state file pinned, so retrying would download the same wrong
            # blob again.
            if expected_sha256 is not None:
                actual_sha256 = file_sha256(tmp_dest)
                if actual_sha256 != expected_sha256:
                    os.remove(tmp_dest)
                    raise SystemExit(
                        f'error: sha256 mismatch for {url}\n'
                        f'  expected: {expected_sha256}\n'
                        f'  actual:   {actual_sha256}\n'
                        'The downloaded file was deleted. The CDN blob does '
                        'not match the checked-in state file; do not build '
                        'with this profile.'
                    )
            os.replace(tmp_dest, dest)
            return
        except (urllib.error.URLError, OSError) as e:
            if os.path.isfile(dest + '.tmp'):
                os.remove(dest + '.tmp')
            if attempt == DOWNLOAD_RETRIES:
                raise SystemExit(f'error: failed to download {url}: {e}') from e
            print(f'attempt {attempt} failed ({e}), retrying')
            time.sleep(DOWNLOAD_RETRY_DELAY_S * attempt)


def cached_file_ok(target, dest, cdn_name, expected_sha256):
    """Whether an already-downloaded file still satisfies the state file."""
    if expected_sha256 is None:
        # Legacy hash-less state file: versioned names are all we have.
        print(f'{target}: {cdn_name} already present')
        return True
    actual_sha256 = file_sha256(dest)
    if actual_sha256 == expected_sha256:
        print(f'{target}: {cdn_name} already present (sha256 verified)')
        return True
    print(
        f'{target}: cached {os.path.basename(dest)} has sha256 '
        f'{actual_sha256}, expected {expected_sha256}; re-downloading'
    )
    os.remove(dest)
    return False


def download_cpp_profile(target):
    cdn_name, expected_sha256 = read_state_file(target)
    dest = os.path.join(PROFILES_DIR, cdn_name)

    if os.path.isfile(dest) and cached_file_ok(
        target, dest, cdn_name, expected_sha256
    ):
        return

    url = CDN_BASE_URL + cdn_name
    print(f'{target}: downloading {url}')
    fetch(url, dest, expected_sha256)
    remove_stale_profiles(target, cdn_name)
    size_mb = os.path.getsize(dest) / (1024 * 1024)
    print(f'{target}: wrote {cdn_name} ({size_mb:.1f} MB)')


def download_v8_builtins_profile():
    cdn_name, expected_sha256 = read_state_file(V8_BUILTINS_TARGET)
    dest = os.path.join(PROFILES_DIR, V8_BUILTINS_LOCAL_NAME)
    version_marker = dest + '.version'

    # Fixed local name: a version marker tracks which CDN profile it holds.
    if os.path.isfile(dest) and os.path.isfile(version_marker):
        with open(version_marker, encoding='utf-8') as f:
            marker_ok = f.read().strip() == cdn_name
        if marker_ok and cached_file_ok(
            V8_BUILTINS_TARGET, dest, cdn_name, expected_sha256
        ):
            return

    url = CDN_BASE_URL + cdn_name
    print(f'{V8_BUILTINS_TARGET}: downloading {url}')
    fetch(url, dest, expected_sha256)
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
