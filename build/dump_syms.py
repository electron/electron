from __future__ import print_function

import collections
import os
import subprocess
import sys

# The BINARY_INFO tuple describes a binary as dump_syms identifies it.
BINARY_INFO = collections.namedtuple('BINARY_INFO',
                                     ['platform', 'arch', 'hash', 'name'])

def get_module_info(dump_syms, binary):
  header_info = subprocess.check_output([dump_syms, '-i', binary]).splitlines()[0]
  # header info is of the form "MODULE $PLATFORM $ARCH $HASH $BINARY"
  info_split = header_info.strip().split(' ', 4)
  if len(info_split) != 5 or info_split[0] != 'MODULE':
    return None
  return BINARY_INFO(*info_split[1:])

def get_symbol_path(dump_syms, binary):
  module_info = get_module_info(dump_syms, binary)
  if not module_info:
    raise Exception("Couldn't get module info for binary '{}'".format(binary))
  return os.path.join(module_info.name, module_info.hash, module_info.name + ".sym")

def mkdir_p(path):
  """Simulates mkdir -p."""
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else: raise

def main(dump_syms, binary, out_dir, stamp_file, dsym_file=None):
  symbol_file = os.path.join(out_dir, get_symbol_path(dump_syms, binary))
  mkdir_p(os.path.dirname(symbol_file))
  with open(symbol_file, 'w') as outfileobj:
    args = [dump_syms]
    if dsym_file:
      args += ["-g", dsym_file]
    args += [binary]
    subprocess.check_call(args, stdout=outfileobj)

  with open(stamp_file, 'w'):
    pass

if __name__ == '__main__':
  main(*sys.argv[1:])
