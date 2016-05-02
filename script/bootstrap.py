#!/usr/bin/env python

import argparse
import os
import subprocess
import sys

from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, PLATFORM, \
                       enable_verbose_mode, is_verbose_mode, get_target_arch
from lib.util import execute_stdout, get_atom_shell_version, scoped_cwd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
PYTHON_26_URL = 'https://chromium.googlesource.com/chromium/deps/python_26'

if os.environ.has_key('CI'):
  NPM = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'npm')
else:
  NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'


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

  update_submodules()

  libcc_source_path = args.libcc_source_path
  libcc_shared_library_path = args.libcc_shared_library_path
  libcc_static_library_path = args.libcc_static_library_path

  # Redirect to use local libchromiumcontent build.
  if args.build_libchromiumcontent:
    build_libchromiumcontent(args.verbose, args.target_arch, defines)
    dist_dir = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                            'libchromiumcontent', 'dist', 'main')
    libcc_source_path = os.path.join(dist_dir, 'src')
    libcc_shared_library_path = os.path.join(dist_dir, 'shared_library')
    libcc_static_library_path = os.path.join(dist_dir, 'static_library')

  if PLATFORM != 'win32' and not args.disable_clang and args.clang_dir == '':
    update_clang()

  setup_python_libs()
  update_node_modules('.')
  bootstrap_brightray(args.dev, args.url, args.target_arch,
                      libcc_source_path, libcc_shared_library_path,
                      libcc_static_library_path)

  if PLATFORM == 'linux':
    download_sysroot(args.target_arch)

  create_chrome_version_h()
  touch_config_gypi()
  run_update(defines)
  update_electron_modules('spec', args.target_arch)


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
  parser.add_argument('--target_arch', default=get_target_arch(),
                      help='Manually specify the arch to build for')
  parser.add_argument('--clang_dir', default='', help='Path to clang binaries')
  parser.add_argument('--disable_clang', action='store_true',
                      help='Use compilers other than clang for building')
  parser.add_argument('--build_libchromiumcontent', action='store_true',
                      help='Build local version of libchromiumcontent')
  parser.add_argument('--libcc_source_path', required=False,
                      help='The source path of libchromiumcontent. ' \
                           'NOTE: All options of libchromiumcontent are ' \
                           'required OR let electron choose it')
  parser.add_argument('--libcc_shared_library_path', required=False,
                      help='The shared library path of libchromiumcontent.')
  parser.add_argument('--libcc_static_library_path', required=False,
                      help='The static library path of libchromiumcontent.')
  return parser.parse_args()


def args_to_defines(args):
  defines = ''
  if args.disable_clang:
    defines += ' clang=0'
  if args.clang_dir:
    defines += ' make_clang_dir=' + args.clang_dir
    defines += ' clang_use_chrome_plugins=0'
  return defines


def check_root():
  if os.geteuid() == 0:
    print "We suggest not running this as root, unless you're really sure."
    choice = raw_input("Do you want to continue? [y/N]: ")
    if choice not in ('y', 'Y'):
      sys.exit(0)


def update_submodules():
  execute_stdout(['git', 'submodule', 'sync'])
  execute_stdout(['git', 'submodule', 'update', '--init', '--recursive'])


def setup_python_libs():
  for lib in ('requests', 'boto'):
    with scoped_cwd(os.path.join(VENDOR_DIR, lib)):
      execute_stdout([sys.executable, 'setup.py', 'build'])


def bootstrap_brightray(is_dev, url, target_arch, libcc_source_path,
                        libcc_shared_library_path,
                        libcc_static_library_path):
  bootstrap = os.path.join(VENDOR_DIR, 'brightray', 'script', 'bootstrap')
  args = [
    '--commit', LIBCHROMIUMCONTENT_COMMIT,
    '--target_arch', target_arch,
    url
  ]
  if is_dev:
    args = ['--dev'] + args
  if (libcc_source_path != None and
      libcc_shared_library_path != None and
      libcc_static_library_path != None):
    args += ['--libcc_source_path', libcc_source_path,
             '--libcc_shared_library_path', libcc_shared_library_path,
             '--libcc_static_library_path', libcc_static_library_path]
  execute_stdout([sys.executable, bootstrap] + args)


def update_node_modules(dirname, env=None):
  if env is None:
    env = os.environ
  if PLATFORM == 'linux':
    # Use prebuilt clang for building native modules.
    llvm_dir = os.path.join(SOURCE_ROOT, 'vendor', 'llvm-build',
                            'Release+Asserts', 'bin')
    env['CC']  = os.path.join(llvm_dir, 'clang')
    env['CXX'] = os.path.join(llvm_dir, 'clang++')
    env['npm_config_clang'] = '1'
  with scoped_cwd(dirname):
    args = [NPM, 'install']
    if is_verbose_mode():
      args += ['--verbose']
    # Ignore npm install errors when running in CI.
    if os.environ.has_key('CI'):
      try:
        execute_stdout(args, env)
      except subprocess.CalledProcessError:
        pass
    else:
      execute_stdout(args, env)


def update_electron_modules(dirname, target_arch):
  env = os.environ.copy()
  env['npm_config_arch']    = target_arch
  env['npm_config_target']  = get_atom_shell_version()
  env['npm_config_disturl'] = 'https://atom.io/download/atom-shell'
  update_node_modules(dirname, env)


def update_win32_python():
  with scoped_cwd(VENDOR_DIR):
    if not os.path.exists('python_26'):
      execute_stdout(['git', 'clone', PYTHON_26_URL])


def build_libchromiumcontent(verbose, target_arch, defines):
  args = [os.path.join(SOURCE_ROOT, 'script', 'build-libchromiumcontent.py')]
  if verbose:
    args += ['-v']
  if defines:
    args += ['--defines', defines]
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
                  '--arch', target_arch])

def create_chrome_version_h():
  version_file = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                              'libchromiumcontent', 'VERSION')
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


def run_update(defines):
  update = os.path.join(SOURCE_ROOT, 'script', 'update.py')
  execute_stdout([sys.executable, update, '--defines', defines])


if __name__ == '__main__':
  sys.exit(main())
