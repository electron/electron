#!/usr/bin/env python

import subprocess
import sys

from lib.util import scoped_cwd


class GNProject:
  def __init__(self, out_dir):
    self.out_dir = out_dir

  def _get_executable_name(self):
    if sys.platform == 'win32':
      return 'gn.bat'

    return 'gn'

  def run(self, command_name, command_args):
    with scoped_cwd(self.out_dir):
      complete_args = [self._get_executable_name(), command_name, '.'] + \
                      command_args
      return subprocess.check_output(complete_args)

  def args(self):
    return GNArgs(self)


class GNArgs:
  def __init__(self, project):
    self.project = project

  def _get_raw_value(self, name):
    # E.g. 'version = "1.0.0"\n'
    raw_output = self.project.run('args',
                                  ['--list={}'.format(name), '--short'])

    # E.g. 'version = "1.0.0"'
    name_with_raw_value = raw_output[:-1]

    # E.g. ['version', '"1.0.0"']
    name_and_raw_value = name_with_raw_value.split(' = ')

    raw_value = name_and_raw_value[1]
    return raw_value

  def get_string(self, name):
    # Expects to get a string in double quotes, e.g. '"some_value"'.
    raw_value = self._get_raw_value(name)

    # E.g. 'some_value' (without enclosing quotes).
    value = raw_value[1:-1]
    return value

  def get_boolean(self, name):
    # Expects to get a 'true' or a 'false' string.
    raw_value = self._get_raw_value(name)

    if raw_value == 'true':
      return True

    if raw_value == 'false':
      return False

    return None


def gn(out_dir):
  return GNProject(out_dir)
