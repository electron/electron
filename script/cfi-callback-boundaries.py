#!/usr/bin/env python3

import argparse
import os
import pathlib
import subprocess
import sys


ELECTRON_ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC_ROOT = ELECTRON_ROOT.parent
VERIFIER = ELECTRON_ROOT / 'build/cfi/verify_callback_boundaries.py'
MANIFEST = ELECTRON_ROOT / 'build/cfi/ir_callback_manifest.txt'
CLANG_ROOT = SRC_ROOT / 'third_party/llvm-build/Release+Asserts/bin'
GN = SRC_ROOT / 'buildtools/linux64/gn'
VERIFY_TARGET = 'electron:verify_cfi_callback_boundaries'


def run(command, *, capture=False, check=True):
    result = subprocess.run(
        [str(part) for part in command],
        cwd=ELECTRON_ROOT,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
        check=False,
    )
    if check and result.returncode != 0:
        if capture:
            sys.stdout.write(result.stdout)
            sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)
    return result


def active_out_name():
    override = os.environ.get('ELECTRON_OUT_DIR')
    if override:
        return override
    try:
        result = run(['e', 'show', 'out'], capture=True)
    except FileNotFoundError:
        raise SystemExit(
            'Cannot find the e command. Install @electron/build-tools or pass '
            '--out-dir.'
        ) from None
    return result.stdout.strip()


def resolve_out_dir(value):
    value = value or active_out_name()
    path = pathlib.Path(value)
    if path.is_absolute():
        out_dir = path
    elif len(path.parts) > 1:
        out_dir = SRC_ROOT / path
    else:
        out_dir = SRC_ROOT / 'out' / path
    out_dir = out_dir.resolve()
    if not out_dir.is_dir():
        raise SystemExit(
            f'Build output does not exist: {out_dir}\n'
            'Select a build-tools config with e use or pass --out-dir.'
        )
    return out_dir


def verifier_command(out_dir, *, update=False):
    command = [
        sys.executable,
        VERIFIER,
        '--clang',
        CLANG_ROOT / 'clang',
        '--llvm-ar',
        CLANG_ROOT / 'llvm-ar',
        '--build-dir',
        out_dir,
        '--ninja-objects',
        (
            f'v8={out_dir}/obj/v8/v8_base_without_compiler.ninja'
            '::phony/v8/v8_base_without_compiler.linkdeps'
        ),
        '--thin-archive',
        (
            'libuv='
            f'{out_dir}/obj/third_party/electron_node/deps/uv/libuv.a'
        ),
        '--manifest',
        MANIFEST,
    ]
    if update:
        command.append('--update')
    return command


def print_review_help():
    print(
        '''
How to read the diff:
  icall_checks > 0    CFI protects a function-pointer call. This is the safe
                      default.
  unchecked_calls > 0 The function has indirect calls without a matching CFI
                      icall or vcall check. Accept this only when an
                      uninstrumented native addon can supply the callback.

Before updating:
  1. Locate every added or changed function in V8 or libuv.
  2. Identify who supplies its callback.
  3. Keep internal callbacks protected.
  4. For addon callbacks, use only a narrow dispatcher suppression and add a
     runtime addon boundary test.

After review, run:
  python3 script/cfi-callback-boundaries.py update
'''.strip(),
        file=sys.stderr,
    )


def check_inventory(out_dir):
    print(f'Checking CFI callback boundaries in {out_dir}...')
    result = run(verifier_command(out_dir), check=False)
    if result.returncode != 0:
        print_review_help()
    return result.returncode


def update_inventory(out_dir, *, build):
    print(f'Regenerating {MANIFEST.relative_to(ELECTRON_ROOT)} from {out_dir}...')
    run(verifier_command(out_dir, update=True))
    print('Manifest regenerated. Review it before committing:')
    print('  git diff -- build/cfi/ir_callback_manifest.txt')
    if build:
        return build_verifier()
    print('Then validate it with:')
    print('  python3 script/cfi-callback-boundaries.py build')
    return 0


def build_verifier():
    print(f'Building {VERIFY_TARGET} with the active build-tools config...')
    return run(['e', 'build', '-t', VERIFY_TARGET], check=False).returncode


def show_status(out_dir):
    print(f'Build output: {out_dir}')
    for name in (
        'is_cfi',
        'use_cfi_icall',
        'use_thin_lto',
        'thin_lto_enable_optimizations',
    ):
        result = run(
            [GN, 'args', out_dir, f'--list={name}', '--short'],
            capture=True,
            check=False,
        )
        value = result.stdout.strip() or f'{name}: unavailable'
        print(value)
    required = [
        out_dir / 'obj/v8/v8_base_without_compiler.ninja',
        out_dir / 'obj/third_party/electron_node/deps/uv/libuv.a',
    ]
    missing = [path for path in required if not path.is_file()]
    if missing:
        print('Missing build inputs:', file=sys.stderr)
        for path in missing:
            print(f'  {path}', file=sys.stderr)
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(
        description='Review and update Electron CFI callback boundaries.'
    )
    parser.add_argument(
        '--out-dir',
        help=(
            'Build output name or path. Defaults to ELECTRON_OUT_DIR, then '
            '`e show out`.'
        ),
    )
    subparsers = parser.add_subparsers(dest='command', required=True)
    subparsers.add_parser('check', help='Compare current IR with the manifest.')
    update_parser = subparsers.add_parser(
        'update', help='Regenerate the manifest after reviewing changes.'
    )
    update_parser.add_argument(
        '--build',
        action='store_true',
        help='Validate the GN target after regenerating.',
    )
    subparsers.add_parser(
        'build', help='Build the integrated verifier target.'
    )
    subparsers.add_parser(
        'status', help='Show the selected output and relevant GN flags.'
    )
    args = parser.parse_args()

    if args.command == 'build':
        return build_verifier()

    out_dir = resolve_out_dir(args.out_dir)
    if args.command == 'check':
        return check_inventory(out_dir)
    if args.command == 'update':
        return update_inventory(out_dir, build=args.build)
    if args.command == 'status':
        return show_status(out_dir)
    parser.error(f'unknown command: {args.command}')
    return 2


if __name__ == '__main__':
    sys.exit(main())
