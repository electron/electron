from __future__ import unicode_literals
import contextlib
import sys
import os
import optparse
import json
import subprocess

sys.path.append("%s/../../build" % os.path.dirname(os.path.realpath(__file__)))

import find_depot_tools
from vs_toolchain import \
    SetEnvironmentAndGetRuntimeDllDirs, \
    SetEnvironmentAndGetSDKDir, \
    NormalizePath

sys.path.append("%s/win_toolchain" % find_depot_tools.add_depot_tools_to_path())

from get_toolchain_if_necessary import CalculateHash


@contextlib.contextmanager
def cwd(directory):
    curdir = os.getcwd()
    try:
        os.chdir(directory)
        yield
    finally:
        os.chdir(curdir)


def calculate_hash(root):
    with cwd(root):
        return CalculateHash('.', None)

def windows_installed_software():
    powershell_command = [
        "Get-CimInstance",
        "-Namespace",
        "root\cimv2",
        "-Class",
        "Win32_product",
        "|",
        "Select",
        "vendor,",
        "description,",
        "@{l='install_location';e='InstallLocation'},",
        "@{l='install_date';e='InstallDate'},",
        "@{l='install_date_2';e='InstallDate2'},",
        "caption,",
        "version,",
        "name,",
        "@{l='sku_number';e='SKUNumber'}",
        "|",
        "ConvertTo-Json",
    ]

    proc = subprocess.Popen(
        ["powershell.exe", "-Command", "-"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
    )

    stdout, _ = proc.communicate(" ".join(powershell_command).encode("utf-8"))

    if proc.returncode != 0:
        raise RuntimeError("Failed to get list of installed software")

    # Filter out missing keys
    return list(
        map(
            lambda info: {k: info[k] for k in info if info[k]},
            json.loads(stdout.decode("utf-8")),
        )
    )


def windows_profile():
    runtime_dll_dirs = SetEnvironmentAndGetRuntimeDllDirs()
    win_sdk_dir = SetEnvironmentAndGetSDKDir()
    path = NormalizePath(os.environ['GYP_MSVS_OVERRIDE_PATH'])

    # since current windows executable are symbols path dependant,
    # profile the current directory too
    return {
        'pwd': os.getcwd(),
        'installed_software': windows_installed_software(),
        'sdks': [
            {'name': 'vs', 'path': path, 'hash': calculate_hash(path)},
            {
                'name': 'wsdk',
                'path': win_sdk_dir,
                'hash': calculate_hash(win_sdk_dir),
            },
        ],
        'runtime_lib_dirs': runtime_dll_dirs,
    }


def main(options):
    if sys.platform == 'win32':
        with open(options.output_json, 'w') as f:
            json.dump(windows_profile(), f)
    else:
        raise OSError("Unsupported OS")


if __name__ == '__main__':
  parser = optparse.OptionParser()
  parser.add_option('--output-json', metavar='FILE', default='profile.json',
                    help='write information about toolchain to FILE')
  opts, args = parser.parse_args()
  sys.exit(main(opts))
