#!/usr/bin/env python
"""A wrapper script around clang-format, suitable for linting multiple files
and to use for continuous integration.
This is an alternative API for the clang-format command line.
It runs over multiple files and directories in parallel.
A diff output is produced and a sensible exit code is returned.
"""

from __future__ import print_function, unicode_literals

import argparse
import codecs
import difflib
import fnmatch
import io
import multiprocessing
import os
import posixpath
import signal
import subprocess
import sys
import traceback
import tempfile

from functools import partial
from lib.util import get_buildtools_executable

DEFAULT_EXTENSIONS = 'c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx,mm'


class ExitStatus:
    SUCCESS = 0
    DIFF = 1
    TROUBLE = 2


def list_files(files, recursive=False, extensions=None, exclude=None):
    if extensions is None:
        extensions = []
    if exclude is None:
        exclude = []

    out = []
    for f in files:
        if recursive and os.path.isdir(f):
            for dirpath, dnames, fnames in os.walk(f):
                fpaths = [os.path.join(dirpath, fname) for fname in fnames]
                for pattern in exclude:
                    dnames[:] = [
                        x for x in dnames
                        if
                        not fnmatch.fnmatch(os.path.join(dirpath, x), pattern)
                    ]
                    fpaths = [
                        x for x in fpaths if not fnmatch.fnmatch(x, pattern)
                    ]
                for fp in fpaths:
                    ext = os.path.splitext(f)[1][1:]
                    print(ext)
                    if ext in extensions:
                        out.append(fp)
        else:
            ext = os.path.splitext(f)[1][1:]
            if ext in extensions:
                out.append(f)
    return out


def make_diff(diff_file, original, reformatted):
    return list(
        difflib.unified_diff(
            original,
            reformatted,
            fromfile='a/{}'.format(diff_file),
            tofile='b/{}'.format(diff_file),
            n=3))


class DiffError(Exception):
    def __init__(self, message, errs=None):
        super(DiffError, self).__init__(message)
        self.errs = errs or []


class UnexpectedError(Exception):
    def __init__(self, message, exc=None):
        super(UnexpectedError, self).__init__(message)
        self.formatted_traceback = traceback.format_exc()
        self.exc = exc


def run_clang_format_diff_wrapper(args, file_name):
    try:
        ret = run_clang_format_diff(args, file_name)
        return ret
    except DiffError:
        raise
    except Exception as e:
        raise UnexpectedError('{}: {}: {}'.format(
            file_name, e.__class__.__name__, e), e)


def run_clang_format_diff(args, file_name):
    try:
        with io.open(file_name, 'r', encoding='utf-8') as f:
            original = f.readlines()
    except IOError as exc:
        raise DiffError(str(exc))
    invocation = [args.clang_format_executable, file_name]
    if args.fix:
        invocation.append('-i')
    try:
        proc = subprocess.Popen(
            ' '.join(invocation),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            shell=True)
    except OSError as exc:
        raise DiffError(str(exc))
    proc_stdout = proc.stdout
    proc_stderr = proc.stderr
    if sys.version_info[0] == 3:
        proc_stdout = proc_stdout.detach()
        proc_stderr = proc_stderr.detach()
    # make the pipes compatible with Python 3,
    # reading lines should output unicode
    encoding = 'utf-8'
    proc_stdout = codecs.getreader(encoding)(proc_stdout)
    proc_stderr = codecs.getreader(encoding)(proc_stderr)
    outs = list(proc_stdout.readlines())
    errs = list(proc_stderr.readlines())
    proc.wait()
    if proc.returncode:
        raise DiffError("clang-format exited with status {}: '{}'".format(
            proc.returncode, file_name), errs)
    if args.fix:
        return None, errs
    if sys.platform == 'win32':
        file_name = file_name.replace(os.sep, posixpath.sep)
    return make_diff(file_name, original, outs), errs


def bold_red(s):
    return '\x1b[1m\x1b[31m' + s + '\x1b[0m'


def colorize(diff_lines):
    def bold(s):
        return '\x1b[1m' + s + '\x1b[0m'

    def cyan(s):
        return '\x1b[36m' + s + '\x1b[0m'

    def green(s):
        return '\x1b[32m' + s + '\x1b[0m'

    def red(s):
        return '\x1b[31m' + s + '\x1b[0m'

    for line in diff_lines:
        if line[:4] in ['--- ', '+++ ']:
            yield bold(line)
        elif line.startswith('@@ '):
            yield cyan(line)
        elif line.startswith('+'):
            yield green(line)
        elif line.startswith('-'):
            yield red(line)
        else:
            yield line


def print_diff(diff_lines, use_color):
    if use_color:
        diff_lines = colorize(diff_lines)
    if sys.version_info[0] < 3:
        sys.stdout.writelines((l.encode('utf-8') for l in diff_lines))
    else:
        sys.stdout.writelines(diff_lines)


def print_trouble(prog, message, use_colors):
    error_text = 'error:'
    if use_colors:
        error_text = bold_red(error_text)
    print("{}: {} {}".format(prog, error_text, message), file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--clang-format-executable',
        metavar='EXECUTABLE',
        help='path to the clang-format executable',
        default=get_buildtools_executable('clang-format'))
    parser.add_argument(
        '--extensions',
        help='comma separated list of file extensions (default: {})'.format(
            DEFAULT_EXTENSIONS),
        default=DEFAULT_EXTENSIONS)
    parser.add_argument(
        '--fix',
        help='if specified, reformat files in-place',
        action='store_true')
    parser.add_argument(
        '-r',
        '--recursive',
        action='store_true',
        help='run recursively over directories')
    parser.add_argument('files', metavar='file', nargs='+')
    parser.add_argument(
        '-q',
        '--quiet',
        action='store_true')
    parser.add_argument(
        '-c',
        '--changed',
        action='store_true',
        help='only run on changed files')
    parser.add_argument(
        '-j',
        metavar='N',
        type=int,
        default=0,
        help='run N clang-format jobs in parallel'
        ' (default number of cpus + 1)')
    parser.add_argument(
        '--color',
        default='auto',
        choices=['auto', 'always', 'never'],
        help='show colored diff (default: auto)')
    parser.add_argument(
        '-e',
        '--exclude',
        metavar='PATTERN',
        action='append',
        default=[],
        help='exclude paths matching the given glob-like pattern(s)'
        ' from recursive search')

    args = parser.parse_args()

    # use default signal handling, like diff return SIGINT value on ^C
    # https://bugs.python.org/issue14229#msg156446
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    try:
        signal.SIGPIPE
    except AttributeError:
        # compatibility, SIGPIPE does not exist on Windows
        pass
    else:
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    colored_stdout = False
    colored_stderr = False
    if args.color == 'always':
        colored_stdout = True
        colored_stderr = True
    elif args.color == 'auto':
        colored_stdout = sys.stdout.isatty()
        colored_stderr = sys.stderr.isatty()

    retcode = ExitStatus.SUCCESS

    parse_files = []
    if args.changed:
        popen = subprocess.Popen(
            'git diff --name-only --cached',
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=True
        )
        for line in popen.stdout:
            file_name = line.rstrip()
            # don't check deleted files
            if os.path.isfile(file_name):
                parse_files.append(file_name)

    else:
        parse_files = args.files

    files = list_files(
        parse_files,
        recursive=args.recursive,
        exclude=args.exclude,
        extensions=args.extensions.split(','))

    if not files:
        return 0

    njobs = args.j
    if njobs == 0:
        njobs = multiprocessing.cpu_count() + 1
    njobs = min(len(files), njobs)

    if not args.fix:
        patch_file = tempfile.NamedTemporaryFile(delete=False,
                                                 prefix='electron-format-')

    if njobs == 1:
        # execute directly instead of in a pool,
        # less overhead, simpler stacktraces
        it = (run_clang_format_diff_wrapper(args, file) for file in files)
        pool = None
    else:
        pool = multiprocessing.Pool(njobs)
        it = pool.imap_unordered(
            partial(run_clang_format_diff_wrapper, args), files)
    while True:
        try:
            outs, errs = next(it)
        except StopIteration:
            break
        except DiffError as e:
            print_trouble(parser.prog, str(e), use_colors=colored_stderr)
            retcode = ExitStatus.TROUBLE
            sys.stderr.writelines(e.errs)
        except UnexpectedError as e:
            print_trouble(parser.prog, str(e), use_colors=colored_stderr)
            sys.stderr.write(e.formatted_traceback)
            retcode = ExitStatus.TROUBLE
            # stop at the first unexpected error,
            # something could be very wrong,
            # don't process all files unnecessarily
            if pool:
                pool.terminate()
            break
        else:
            sys.stderr.writelines(errs)
            if outs == []:
                continue
            if not args.fix:
                if not args.quiet:
                    print_diff(outs, use_color=colored_stdout)
                    for line in outs:
                        patch_file.write(line.encode('utf-8'))
                    patch_file.write('\n'.encode('utf-8'))
                if retcode == ExitStatus.SUCCESS:
                    retcode = ExitStatus.DIFF

    if not args.fix:
        if patch_file.tell() == 0:
          patch_file.close()
          os.unlink(patch_file.name)
        else:
          print("\nTo patch these files, run:\n$ git apply {}\n"
                .format(patch_file.name))

    return retcode


if __name__ == '__main__':
    sys.exit(main())
