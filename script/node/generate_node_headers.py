#!/usr/bin/env python3
import json
import os
import sys
import shutil

from pathlib import Path

SRC_DIR = Path(__file__).resolve().parents[3]
sys.path.append(os.path.join(SRC_DIR, 'third_party/electron_node/tools'))

import install

class LoadPythonDictionaryError(Exception):
    """Custom exception for errors in LoadPythonDictionary."""

def LoadPythonDictionary(path):
    with open(path, 'r', encoding='utf-8') as f:
        file_string = f.read()
        try:
            file_data = eval(file_string, {'__builtins__': None}, None)
        except SyntaxError as e:
            e.filename = path
            raise
        except Exception as e:
            err_msg = f"Unexpected error while reading {path}: {str(e)}"
            raise LoadPythonDictionaryError(err_msg) from e
        if not isinstance(file_data, dict):
            raise LoadPythonDictionaryError(
                f"{path} does not eval to a dictionary"
            )
        return file_data

def get_out_dir():
    out_dir = 'Testing'
    override = os.environ.get('ELECTRON_OUT_DIR')
    if override is not None:
        out_dir = override
    return os.path.join(SRC_DIR, 'out', out_dir)

if __name__ == '__main__':
    node_root_dir = os.path.join(SRC_DIR, 'third_party/electron_node')
    out = {}

    out['headers'] = []
    def add_headers(_, files, dest_dir):
        if 'src/node.h' in files:
            files = [
                f for f in files
                if f.endswith('.h') and f != 'src/node_version.h'
            ]
        if files:
            dir_index = next(
                (i for i, d in enumerate(out['headers'])
                if d['dest_dir'] == dest_dir),
                -1
            )
            if dir_index != -1:
                out['headers'][dir_index]['files'] += sorted(files)
            else:
                hs = {'files': sorted(files), 'dest_dir': dest_dir}
                out['headers'].append(hs)

    root_gen_dir = os.path.join(get_out_dir(), 'gen')
    config_gypi_path = os.path.join(root_gen_dir, 'config.gypi')
    node_headers_dir = os.path.join(root_gen_dir, 'node_headers')

    options = install.parse_options([
        'install',
        '--root-dir', node_root_dir,
        '--v8-dir', os.path.join(SRC_DIR, 'v8'),
        '--config-gypi-path', config_gypi_path,
        '--headers-only'
    ])
    options.variables['node_use_openssl'] = 'false'
    options.variables['node_shared_libuv'] = 'false'
    # We generate zlib headers in Electron's BUILD.gn.
    options.variables['node_shared_zlib'] = ''
    install.headers(options, add_headers)

    header_groups = []
    for header_group in out['headers']:
        sources = [
            os.path.join(node_root_dir, file)
            for file in header_group['files']
        ]
        outputs = [
            os.path.join(
                node_headers_dir, header_group['dest_dir'],
                os.path.basename(file)
            )
            for file in sources
        ]
        for src, dest in zip(sources, outputs):
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            if os.path.exists(dest):
                if os.path.samefile(src, dest):
                    continue
                os.remove(dest)
            shutil.copyfile(src, dest)

    node_header_file = os.path.join(root_gen_dir, 'node_headers.json')
    with open(node_header_file, 'w', encoding='utf-8') as nhf:
        json_data = json.dumps(
            out, sort_keys=True, indent=2, separators=(',', ': ')
        )
        nhf.write(json_data)
