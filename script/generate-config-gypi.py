#!/usr/bin/env python3

import ast
import argparse
import json
import os
import re
import subprocess
import sys

ELECTRON_DIR = os.path.abspath(os.path.join(__file__, '..', '..'))
NODE_DIR = os.path.join(ELECTRON_DIR, '..', 'third_party', 'electron_node')

def run_node_configure(target_cpu, v8_enable_cppgc_microtask_queue):
  configure = os.path.join(NODE_DIR, 'configure.py')
  args = ['--dest-cpu', target_cpu]
  # Enabled in Chromium's V8, will be disabled on 32bit via
  # common.gypi rules
  args += ['--experimental-enable-pointer-compression']

  # v8_cppgc_microtask_queue affects the public ABI of v8::MicrotaskQueue, so
  # node-gyp built native addons must match how the embedded V8 was compiled.
  if v8_enable_cppgc_microtask_queue:
    args += ['--v8-enable-cppgc-microtask-queue']

  # Work around "No acceptable ASM compiler found" error on some System,
  # it breaks nothing since Electron does not use OpenSSL.
  args += ['--openssl-no-asm']

  # Enable whole-program optimization for electron native modules.
  if sys.platform == "win32":
    args += ['--with-ltcg']
  subprocess.check_call([sys.executable, configure] + args)

def read_node_config_gypi():
  config_gypi = os.path.join(NODE_DIR, 'config.gypi')
  with open(config_gypi, 'r', encoding='utf-8') as file_in:
    content = file_in.read()
    return ast.literal_eval(content)

def read_electron_args():
  all_gn = os.path.join(ELECTRON_DIR, 'build', 'args', 'all.gn')
  args = {}
  with open(all_gn, 'r', encoding='utf-8') as file_in:
    for line in file_in:
      if line.startswith('#'):
        continue
      m = re.match(r'(\w+) = (.+)', line)
      if m is None:
        continue
      args[m.group(1)] = m.group(2)
  return args

def main(target_file, target_cpu, v8_enable_cppgc_microtask_queue):
  run_node_configure(target_cpu, v8_enable_cppgc_microtask_queue)
  config = read_node_config_gypi()
  args = read_electron_args()

  # Remove the generated config.gypi to make the parallel/test-process-config
  # test pass.
  os.remove(os.path.join(NODE_DIR, 'config.gypi'))

  v = config['variables']
  # Electron specific variables:
  v['built_with_electron'] = 1
  v['node_module_version'] = int(args['node_module_version'])
  # Used by certain versions of node-gyp.
  v['build_v8_with_gn'] = 'false'
  # One config.gypi ships in the headers for every platform, so per-OS values
  # for native module builds are expressed as gyp conditions rather than baked.
  v.pop('clang', None)
  v.setdefault('conditions', []).append(['OS=="mac"', {'clang%': 1}])
  # Evaluated after common.gypi's own conditions so it wins; keeps native
  # modules loadable on macOS 13.0 - 13.4, which Chromium still supports.
  config.setdefault('target_defaults', {}).setdefault(
      'target_conditions', []).append(
          ['OS=="mac"', {'xcode_settings': {'MACOSX_DEPLOYMENT_TARGET': '13.0'}}])

  # JSON rather than pprint: node-gyp reads this file by swapping quote
  # characters, which only round-trips the condition strings in this form.
  with open(target_file, 'w+', encoding='utf-8') as file_out:
    file_out.write(json.dumps(config, indent=2, sort_keys=True))

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('target_file')
  parser.add_argument('target_cpu')
  parser.add_argument('--v8-enable-cppgc-microtask-queue',
                      action='store_true',
                      dest='v8_enable_cppgc_microtask_queue',
                      default=False)
  parsed = parser.parse_args()
  sys.exit(main(parsed.target_file, parsed.target_cpu,
                parsed.v8_enable_cppgc_microtask_queue))
