#!/usr/bin/env python3

import os
import subprocess
import sys

TEMPLATE = """
#include "node_native_module.h"
#include "node_internals.h"

namespace node {{

namespace native_module {{

{definitions}

void NativeModuleLoader::LoadEmbedderJavaScriptSource() {{
  {initializers}
}}

}}  // namespace native_module

}}  // namespace node
"""

def main():
  node_path = os.path.abspath(sys.argv[1])
  natives = os.path.abspath(sys.argv[2])
  js_source_files = sys.argv[3:]

  js2c = os.path.join(node_path, 'tools', 'js2c.py')
  subprocess.check_call(
    [sys.executable, js2c] +
    js_source_files +
    ['--only-js', '--target', natives])


if __name__ == '__main__':
  sys.exit(main())
