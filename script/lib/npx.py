import subprocess
import sys


def npx(*npx_args):
    call_args = [__get_executable_name()] + list(npx_args)
    subprocess.check_call(call_args)


def __get_executable_name():
    executable = 'npx'
    if sys.platform == 'win32':
        executable += '.cmd'
    return executable


if __name__ == '__main__':
    npx(*sys.argv[1:])
