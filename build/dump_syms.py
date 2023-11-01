#!/usr/bin/env python3

import collections
import os
import subprocess
import sys
import errno

# The BINARY_INFO tuple describes a binary as dump_syms identifies it.
BINARY_INFO = collections.namedtuple('BINARY_INFO',
                                     ['platform', 'arch', 'hash', 'name'])

def get_module_info(header_info):
  # header info is of the form "MODULE $PLATFORM $ARCH $HASH $BINARY"
  info_split = header_info.strip().split(' ', 4)
  if len(info_split) != 5 or info_split[0] != 'MODULE':
    return None
  return BINARY_INFO(*info_split[1:])

def get_symbol_path(symbol_data):
  module_info = get_module_info(symbol_data[:symbol_data.index('\n')])
  if not module_info:
    raise Exception("Couldn't get module info for binary '{}'".format(binary))
  exe_name = module_info.name.replace('.pdb', '')
  return os.path.join(module_info.name, module_info.hash, exe_name + ".sym")

def mkdir_p(path):
  """Simulates mkdir -p."""
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else: raise

def main(dump_syms, binary, out_dir, stamp_file, dsym_file=None):
  args = [dump_syms]
  if dsym_file:
    args += ["-g", dsym_file]
  args += [binary]

  symbol_data = subprocess.check_output(args).decode(sys.stdout.encoding)
  symbol_path = os.path.join(out_dir, get_symbol_path(symbol_data))
  mkdir_p(os.path.dirname(symbol_path))

  with open(symbol_path, 'w') as out:
    out.write(symbol_data)

  with open(stamp_file, 'w'):
    pass

if __name__ == '__main__':
  main(*sys.argv[1:])
