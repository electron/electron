#!/usr/bin/python3

import os
import sys

NODE_ROOT_DIR = "../../third_party/electron_node"
out_dir = sys.argv[1]
fs_files = sys.argv[2:]

for fs_file in fs_files:
    with open(os.path.join(NODE_ROOT_DIR, fs_file), 'r',
              encoding='utf-8') as f:
        contents = f.read()
        original_fs_file = fs_file.replace('internal/fs/',
                'internal/original-fs/').replace('lib/fs.js',
                'lib/original-fs.js').replace('lib/fs/',
                'lib/original-fs/')

        with open(os.path.join(out_dir, original_fs_file), 'w',
                  encoding='utf-8') as transformed_f:
            transformed_contents = contents.replace('internal/fs/',
                    'internal/original-fs/').replace('require(\'fs',
                    'require(\'original-fs')
            # The asar fs wrapper (lib/node/asar-fs-wrapper.ts) intercepts
            # some functions on the shared `fs` / `fs_dir` bindings so that
            # every entry point of the `fs` module handles archives. It keeps
            # a snapshot of the pristine bindings on the `fs` binding under
            # `_electronOriginalBindings`; original-fs must always use that
            # snapshot (falling back to the live binding when the wrapper is
            # not installed, e.g. ELECTRON_RUN_AS_NODE) so that it keeps
            # behaving like unmodified Node.js.
            for name in ('fs', 'fs_dir'):
                transformed_contents = transformed_contents.replace(
                        f"internalBinding('{name}')",
                        "(internalBinding('fs')._electronOriginalBindings"
                        f"?.{name} ?? internalBinding('{name}'))")
            transformed_f.write(transformed_contents)
