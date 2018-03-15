#!/usr/bin/env python

import argparse
import errno
import os
import re
import subprocess
import sys

from lib.config import BASE_URL, PLATFORM, MIPS64EL_SYSROOT_URL, \
                       MIPS64EL_GCC, MIPS64EL_GCC_URL, enable_verbose_mode, \
                       is_verbose_mode, get_target_arch
from lib.util import execute, execute_stdout, get_electron_version, \
                     scoped_cwd, download, update_node_modules
from tls import check_tls

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
DOWNLOAD_DIR = os.path.join(VENDOR_DIR, 'download')
PYTHON_26_URL = 'https://chromium.googlesource.com/chromium/deps/python_26'

def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  defines = args_to_defines(args)
  if not args.yes and PLATFORM != 'win32':
    check_root()
  if args.verbose:
    enable_verbose_mode()
  if sys.platform == 'cygwin':
    update_win32_python()

  check_tls(args.verbose)

  update_submodules()

  libcc_source_path = args.libcc_source_path
  libcc_shared_library_path = args.libcc_shared_library_path
  libcc_static_library_path = args.libcc_static_library_path

  if args.target_arch == 'mips64el':
    download_mips64el_toolchain()

  if args.target_arch.startswith('arm'):
    download_native_mksnapshot(args.target_arch)

  # Redirect to use local libchromiumcontent build.
  if args.build_release_libcc or args.build_debug_libcc:
    build_libchromiumcontent(args.verbose, args.target_arch,
                             args.build_debug_libcc, args.update_libcc)
    dist_dir = os.path.join(VENDOR_DIR, 'libchromiumcontent', 'dist', 'main')
    libcc_source_path = os.path.join(dist_dir, 'src')
    libcc_shared_library_path = os.path.join(dist_dir, 'shared_library')
    libcc_static_library_path = os.path.join(dist_dir, 'static_library')

  if PLATFORM != 'win32':
    if not args.disable_clang and args.clang_dir == '':
      # Download prebuilt clang binaries.
      update_clang()

  setup_python_libs()
  update_node_modules('.')
  setup_libchromiumcontent(args.dev, args.target_arch, args.url,
                           libcc_source_path, libcc_shared_library_path,
                           libcc_static_library_path)

  if PLATFORM == 'linux' and args.target_arch != 'mips64el':
    download_sysroot(args.target_arch)

  create_chrome_version_h()
  touch_config_gypi()
  run_update(defines, args.msvs)


def parse_args():
  parser = argparse.ArgumentParser(description='Bootstrap this project')
  parser.add_argument('-u', '--url',
                      help='The base URL from which to download '
                      'libchromiumcontent (i.e., the URL you passed to '
                      'libchromiumcontent\'s script/upload script',
                      default=BASE_URL,
                      required=False)
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('-d', '--dev', action='store_true',
                      help='Do not download static_library build')
  parser.add_argument('-y', '--yes', '--assume-yes',
                      action='store_true',
                      help='Run non-interactively by assuming "yes" to all ' \
                           'prompts.')
  parser.add_argument('--msvs', action='store_true',
                      help='Generate Visual Studio project')
  parser.add_argument('--target_arch', default=get_target_arch(),
                      help='Manually specify the arch to build for')
  parser.add_argument('--clang_dir', default='', help='Path to clang binaries')
  parser.add_argument('--disable_clang', action='store_true',
                      help='Use compilers other than clang for building')
  build_libcc = parser.add_mutually_exclusive_group()
  build_libcc.add_argument('--build_release_libcc', action='store_true',
                           help='Build release version of libchromiumcontent')
  build_libcc.add_argument('--build_debug_libcc', action='store_true',
                           help='Build debug version of libchromiumcontent')
  parser.add_argument('--update_libcc', default=False,
                      action='store_true', help=('force gclient invocation to '
                      'update libchromiumcontent'))
  parser.add_argument('--libcc_source_path', required=False,
                      help='The source path of libchromiumcontent. ' \
                           'NOTE: All options of libchromiumcontent are ' \
                           'required OR let electron choose it')
  parser.add_argument('--libcc_shared_library_path', required=False,
                      help='The shared library path of libchromiumcontent.')
  parser.add_argument('--libcc_static_library_path', required=False,
                      help='The static library path of libchromiumcontent.')
  parser.add_argument('--defines', default='',
                      help='The build variables passed to gyp')
  parser.add_argument('--cc_wrapper',
                      help='Sets cc_wrapper for build. E.g. $(which sccache)')
  return parser.parse_args()


def args_to_defines(args):
  defines = args.defines
  if args.disable_clang:
    defines += ' clang=0'
  if args.clang_dir:
    defines += ' make_clang_dir=' + args.clang_dir
    defines += ' clang_use_chrome_plugins=0'
  if args.cc_wrapper is not None:
    defines += ' cc_wrapper=' + args.cc_wrapper
  return defines


def check_root():
  if os.geteuid() == 0:
    print "We suggest not running this as root, unless you're really sure."
    choice = raw_input("Do you want to continue? [y/N]: ")
    if choice not in ('y', 'Y'):
      sys.exit(0)


def update_submodules():
  execute_stdout(['git', 'submodule', 'sync', '--recursive'])
  execute_stdout(['git', 'submodule', 'update', '--init', '--recursive'])


def setup_python_libs():
  for lib in ('requests', 'boto'):
    with scoped_cwd(os.path.join(VENDOR_DIR, lib)):
      execute_stdout([sys.executable, 'setup.py', 'build'])


def setup_libchromiumcontent(is_dev, target_arch, url,
                             libcc_source_path,
                             libcc_shared_library_path,
                             libcc_static_library_path):
  target_dir = os.path.join(DOWNLOAD_DIR, 'libchromiumcontent')
  script = os.path.join(VENDOR_DIR, 'libchromiumcontent', 'script',
                        'download')
  args = ['-f', '-c', get_libchromiumcontent_commit(), '--target_arch',
          target_arch, url, target_dir]
  if (libcc_source_path != None and
      libcc_shared_library_path != None and
      libcc_static_library_path != None):
    args += ['--libcc_source_path', libcc_source_path,
            '--libcc_shared_library_path', libcc_shared_library_path,
            '--libcc_static_library_path', libcc_static_library_path]
    mkdir_p(target_dir)
  else:
    mkdir_p(DOWNLOAD_DIR)
  if is_dev:
    subprocess.check_call([sys.executable, script] + args)
  else:
    subprocess.check_call([sys.executable, script, '-s'] + args)


def update_win32_python():
  with scoped_cwd(VENDOR_DIR):
    if not os.path.exists('python_26'):
      execute_stdout(['git', 'clone', PYTHON_26_URL])


def build_libchromiumcontent(verbose, target_arch, debug,
                             force_update):
  args = [sys.executable,
          os.path.join(SOURCE_ROOT, 'script', 'build-libchromiumcontent.py')]
  if debug:
    args += ['-d']
  if force_update:
    args += ['--force-update']
  if verbose:
    args += ['-v']
  execute_stdout(args + ['--target_arch', target_arch])


def update_clang():
  execute_stdout([os.path.join(SOURCE_ROOT, 'script', 'update-clang.sh')])


def download_sysroot(target_arch):
  if target_arch == 'ia32':
    target_arch = 'i386'
  if target_arch == 'x64':
    target_arch = 'amd64'
  execute_stdout([sys.executable,
                  os.path.join(SOURCE_ROOT, 'script', 'install-sysroot.py'),
                  '--arch', target_arch],
                  cwd=VENDOR_DIR)


def download_mips64el_toolchain():
  # Download sysroot image.
  if not os.path.exists(os.path.join(VENDOR_DIR,
                                     'debian_jessie_mips64-sysroot')):
    tar_name = 'debian_jessie_mips64-sysroot.tar.bz2'
    download(tar_name, MIPS64EL_SYSROOT_URL,
             os.path.join(SOURCE_ROOT, tar_name))
    subprocess.call(['tar', '-jxf', tar_name, '-C', VENDOR_DIR])
    os.remove(tar_name)
  # Download toolchain.
  if not os.path.exists(os.path.join(VENDOR_DIR, MIPS64EL_GCC)):
    tar_name = MIPS64EL_GCC + '.tar.gz'
    download(tar_name, MIPS64EL_GCC_URL, os.path.join(SOURCE_ROOT, tar_name))
    subprocess.check_call(['tar', '-xf', tar_name, '-C', VENDOR_DIR])
    os.remove(tar_name)

def download_native_mksnapshot(arch):
  if not os.path.exists(os.path.join(VENDOR_DIR,
                                     'native_mksnapshot')):
    tar_name = 'native-mksnapshot.tar.bz2'
    url = '{0}/linux/{1}/{2}/{3}'.format(BASE_URL, arch,
           get_libchromiumcontent_commit(), tar_name)
    download(tar_name, url, os.path.join(SOURCE_ROOT, tar_name))
    subprocess.call(['tar', '-jxf', tar_name, '-C', VENDOR_DIR])
    os.remove(tar_name)

def create_chrome_version_h():
  version_file = os.path.join(VENDOR_DIR, 'libchromiumcontent', 'VERSION')
  target_file = os.path.join(SOURCE_ROOT, 'atom', 'common', 'chrome_version.h')
  template_file = os.path.join(SOURCE_ROOT, 'script', 'chrome_version.h.in')

  with open(version_file, 'r') as f:
    version = f.read()
  with open(template_file, 'r') as f:
    template = f.read()
  content = template.replace('{PLACEHOLDER}', version.strip())

  # We update the file only if the content has changed (ignoring line ending
  # differences).
  should_write = True
  if os.path.isfile(target_file):
    with open(target_file, 'r') as f:
      should_write = f.read().replace('r', '') != content.replace('r', '')
  if should_write:
    with open(target_file, 'w') as f:
      f.write(content)


def touch_config_gypi():
  config_gypi = os.path.join(SOURCE_ROOT, 'vendor', 'node', 'config.gypi')
  with open(config_gypi, 'w+') as f:
    content = "\n{'variables':{}}"
    if f.read() != content:
      f.write(content)


def run_update(defines, msvs):
  args = [sys.executable, os.path.join(SOURCE_ROOT, 'script', 'update.py')]
  if defines:
    args += ['--defines', defines]
  if msvs:
    args += ['--msvs']

  execute_stdout(args)


def get_libchromiumcontent_commit():
  commit = os.getenv('LIBCHROMIUMCONTENT_COMMIT')
  if commit:
    return commit

  # Extract full SHA-1 of libcc submodule commit
  output = execute(['git', 'submodule', 'status', 'vendor/libchromiumcontent'])
  commit = re.split('^(?:\s*)([a-f0-9]{40})(?:\s+)', output)[1]
  return commit


def mkdir_p(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise


if __name__ == '__main__':
  sys.exit(main())
