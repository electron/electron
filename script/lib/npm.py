import subprocess
import sys


def npm(*npm_args):
    call_args = [__get_executable_name()] + list(npm_args)
    subprocess.check_call(call_args)


def __get_executable_name():
    executable = 'npm'
    if sys.platform == 'win32':
        executable += '.cmd'
    return executable


if __name__ == '__main__':
    npm(*sys.argv[1:])
