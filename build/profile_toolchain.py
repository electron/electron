from __future__ import with_statement
import contextlib
import sys
import os
import optparse
import json

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
    import win32com.client
    strComputer = "."
    objWMIService = win32com.client.Dispatch("WbemScripting.SWbemLocator")
    objSWbemServices = objWMIService.ConnectServer(strComputer, "root\cimv2")
    colItems = objSWbemServices.ExecQuery("Select * from Win32_Product")
    items = []

    for objItem in colItems:
        item = {}
        if objItem.Caption:
            item['caption'] = objItem.Caption
        if objItem.Caption:
            item['description'] = objItem.Description
        if objItem.InstallDate:
            item['install_date'] = objItem.InstallDate
        if objItem.InstallDate2:
            item['install_date_2'] = objItem.InstallDate2
        if objItem.InstallLocation:
            item['install_location'] = objItem.InstallLocation
        if objItem.Name:
            item['name'] = objItem.Name
        if objItem.SKUNumber:
            item['sku_number'] = objItem.SKUNumber
        if objItem.Vendor:
            item['vendor'] = objItem.Vendor
        if objItem.Version:
            item['version'] = objItem.Version
        items.append(item)

    return items


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
        with open(options.output_json, 'wb') as f:
            json.dump(windows_profile(), f)
    else:
        raise OSError("Unsupported OS")


if __name__ == '__main__':
  parser = optparse.OptionParser()
  parser.add_option('--output-json', metavar='FILE', default='profile.json',
                    help='write information about toolchain to FILE')
  opts, args = parser.parse_args()
  sys.exit(main(opts))
