import os
import subprocess
import sys


def npx(*npx_args):
    npx_env = os.environ.copy()
    npx_env['npm_config_yes'] = 'true'
    call_args = [__get_executable_name()] + list(npx_args)
    subprocess.check_call(call_args, env=npx_env)


def __get_executable_name():
    executable = 'npx'
    if sys.platform == 'win32':
        executable += '.cmd'
    return executable


if __name__ == '__main__':
    npx(*sys.argv[1:])
