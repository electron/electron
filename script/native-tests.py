#!/usr/bin/env python

from __future__ import print_function

import argparse
import os
import subprocess
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
PYYAML_LIB_DIR = os.path.join(VENDOR_DIR, 'pyyaml', 'lib')
sys.path.append(PYYAML_LIB_DIR)
import yaml  #pylint: disable=wrong-import-position,wrong-import-order


class Command:
  LIST = 'list'
  RUN = 'run'

class Verbosity:
  CHATTY = 'chatty'  # stdout and stderr
  ERRORS = 'errors'  # stderr only
  SILENT = 'silent'  # no output

  @staticmethod
  def get_all():
    return [Verbosity.CHATTY, Verbosity.ERRORS, Verbosity.SILENT]

class Platform:
  LINUX = 'linux'
  MAC = 'mac'
  WINDOWS = 'windows'

  @staticmethod
  def get_current():
    platform = sys.platform
    if platform in ('linux', 'linux2'):
      return Platform.LINUX
    if platform == 'darwin':
      return Platform.MAC
    if platform in ('cygwin', 'win32'):
      return Platform.WINDOWS

    assert False, "unexpected current platform '{}'".format(platform)

  @staticmethod
  def get_all():
    return [Platform.LINUX, Platform.MAC, Platform.WINDOWS]

  @staticmethod
  def is_valid(platform):
    return platform in Platform.get_all()

def parse_args():
  parser = argparse.ArgumentParser(description='Run Google Test binaries')

  parser.add_argument('command',
                      choices=[Command.LIST, Command.RUN],
                      help='command to execute')

  parser.add_argument('-b', '--binary', nargs='+', required=False,
                      help='binaries to run')
  parser.add_argument('-c', '--config', required=True,
                      help='path to a tests config')
  parser.add_argument('-t', '--tests-dir', required=False,
                      help='path to a directory with test binaries')
  parser.add_argument('-o', '--output-dir', required=False,
                      help='path to a folder to save tests results')

  verbosity = parser.add_mutually_exclusive_group()
  verbosity.add_argument('-v', '--verbosity', required=False,
                         default=Verbosity.CHATTY,
                         choices=Verbosity.get_all(),
                         help='set verbosity level')
  verbosity.add_argument('-q', '--quiet', required=False, action='store_const',
                         const=Verbosity.ERRORS, dest='verbosity',
                         help='suppress stdout from test binaries')
  verbosity.add_argument('-qq', '--quiet-quiet',
                         # https://youtu.be/bXd-zZLV2i0?t=41s
                         required=False, action='store_const',
                         const=Verbosity.SILENT, dest='verbosity',
                         help='suppress stdout and stderr from test binaries')

  args = parser.parse_args()

  # Additional checks.
  if args.command == Command.RUN and args.tests_dir is None:
    parser.error("specify a path to a dir with test binaries via --tests-dir")

  # Absolutize and check paths.
  # 'config' must exist and be a file.
  args.config = os.path.abspath(args.config)
  if not os.path.isfile(args.config):
    parser.error("file '{}' doesn't exist".format(args.config))

  # 'tests_dir' must exist and be a directory.
  if args.tests_dir is not None:
    args.tests_dir = os.path.abspath(args.tests_dir)
    if not os.path.isdir(args.tests_dir):
      parser.error("directory '{}' doesn't exist".format(args.tests_dir))

  # 'output_dir' must exist and be a directory.
  if args.output_dir is not None:
    args.output_dir = os.path.abspath(args.output_dir)
    if not os.path.isdir(args.output_dir):
      parser.error("directory '{}' doesn't exist".format(args.output_dir))

  return args


def main():
  args = parse_args()
  tests_list = TestsList(args.config, args.tests_dir)

  if args.command == Command.LIST:
    all_binaries_names = tests_list.get_for_current_platform()
    print('\n'.join(all_binaries_names))
    return 0

  if args.command == Command.RUN:
    if args.binary is not None:
      return tests_list.run(args.binary, args.output_dir, args.verbosity)
    else:
      return tests_list.run_all(args.output_dir, args.verbosity)

  assert False, "unexpected command '{}'".format(args.command)


class TestsList():
  def __init__(self, config_path, tests_dir):
    self.config_path = config_path
    self.tests_dir = tests_dir

    # A dict with binary names (e.g. 'base_unittests') as keys
    # and various test data as values of dict type.
    self.tests = TestsList.__get_tests_list(config_path)

  def __len__(self):
    return len(self.tests)

  def get_for_current_platform(self):
    all_binaries = self.tests.keys()

    supported_binaries = filter(self.__platform_supports, all_binaries)

    return supported_binaries

  def run(self, binaries, output_dir=None, verbosity=Verbosity.CHATTY):
    # Don't run anything twice.
    binaries = set(binaries)

    # First check that all names are present in the config.
    for binary_name in binaries:
      if binary_name not in self.tests:
        raise Exception("binary {0} not found in config '{1}'".format(
            binary_name, self.config_path))

    # Respect the "platform" setting.
    for binary_name in binaries:
      if not self.__platform_supports(binary_name):
        raise Exception(
            "binary {0} cannot be run on {1}, check the config".format(
                binary_name, Platform.get_current()))

    suite_returncode = sum(
        [self.__run(binary, output_dir, verbosity) for binary in binaries])
    return suite_returncode

  def run_only(self, binary_name, output_dir=None, verbosity=Verbosity.CHATTY):
    return self.run([binary_name], output_dir, verbosity)

  def run_all(self, output_dir=None, verbosity=Verbosity.CHATTY):
    return self.run(self.get_for_current_platform(), output_dir, verbosity)

  @staticmethod
  def __get_tests_list(config_path):
    tests_list = {}
    config_data = TestsList.__get_config_data(config_path)

    for data_item in config_data['tests']:
      (binary_name, test_data) = TestsList.__get_test_data(data_item)
      tests_list[binary_name] = test_data

    return tests_list

  @staticmethod
  def __get_config_data(config_path):
    with open(config_path, 'r') as stream:
      return yaml.load(stream)

  @staticmethod
  def __expand_shorthand(value):
    """ Treat a string as {'string_value': None}."""
    if isinstance(value, dict):
      return value

    if isinstance(value, basestring):
      return {value: None}

    assert False, "unexpected shorthand type: {}".format(type(value))

  @staticmethod
  def __make_a_list(value):
    """Make a list if not already a list."""
    if isinstance(value, list):
        return value
    return [value]

  @staticmethod
  def __merge_nested_lists(value):
    """Converts a dict of lists to a list."""
    if isinstance(value, list):
        return value

    if isinstance(value, dict):
      # It looks ugly as hell, but it does the job.
      return [list_item for key in value for list_item in value[key]]

    assert False, "unexpected type for list merging: {}".format(type(value))

  def __platform_supports(self, binary_name):
    return Platform.get_current() in self.tests[binary_name]['platforms']

  @staticmethod
  def __get_test_data(data_item):
    data_item = TestsList.__expand_shorthand(data_item)

    binary_name = data_item.keys()[0]
    test_data = {
      'excluded_tests': None,
      'platforms': Platform.get_all()
    }

    configs = data_item[binary_name]
    if configs is not None:
      # List of excluded tests.
      if 'disabled' in configs:
        excluded_tests = TestsList.__merge_nested_lists(configs['disabled'])
        test_data['excluded_tests'] = excluded_tests

      # List of platforms to run the tests on.
      if 'platform' in configs:
          platforms = TestsList.__make_a_list(configs['platform'])

          for platform in platforms:
            assert Platform.is_valid(platform), \
                "platform '{0}' is not supported, check {1} config" \
                    .format(platform, binary_name)

          test_data['platforms'] = platforms

    return (binary_name, test_data)

  def __run(self, binary_name, output_dir, verbosity):
    binary_path = os.path.join(self.tests_dir, binary_name)
    test_binary = TestBinary(binary_path)

    test_data = self.tests[binary_name]
    excluded_tests = test_data['excluded_tests']

    output_file_path = TestsList.__get_output_path(binary_name, output_dir)

    return test_binary.run(excluded_tests=excluded_tests,
                           output_file_path=output_file_path,
                           verbosity=verbosity)

  @staticmethod
  def __get_output_path(binary_name, output_dir=None):
    if output_dir is None:
      return None

    return os.path.join(output_dir, "results_{}.xml".format(binary_name))


class TestBinary():
  # Is only used when writing to a file.
  output_format = 'xml'

  def __init__(self, binary_path):
    self.binary_path = binary_path

  def run(self, excluded_tests=None, output_file_path=None,
      verbosity=Verbosity.CHATTY):
    gtest_filter = ""
    if excluded_tests is not None and len(excluded_tests) > 0:
      excluded_tests_string = TestBinary.__format_excluded_tests(
          excluded_tests)
      gtest_filter = "--gtest_filter={}".format(excluded_tests_string)

    gtest_output = ""
    if output_file_path is not None:
      gtest_output = "--gtest_output={0}:{1}".format(TestBinary.output_format,
                                                     output_file_path)

    args = [self.binary_path, gtest_filter, gtest_output]
    stdout, stderr = TestBinary.__get_stdout_and_stderr(verbosity)

    returncode = 0
    try:
      returncode = subprocess.call(args, stdout=stdout, stderr=stderr)
    except Exception as exception:
      if verbosity in (Verbosity.CHATTY, Verbosity.ERRORS):
        print("An error occurred while running '{}':".format(self.binary_path),
            '\n', exception, file=sys.stderr)
      returncode = 1

    return returncode

  @staticmethod
  def __format_excluded_tests(excluded_tests):
    return "-" + ":".join(excluded_tests)

  @staticmethod
  def __get_stdout_and_stderr(verbosity):
    stdout = stderr = None

    if verbosity in (Verbosity.ERRORS, Verbosity.SILENT):
      devnull = open(os.devnull, 'w')
      stdout = devnull
      if verbosity == Verbosity.SILENT:
        stderr = devnull

    return (stdout, stderr)


if __name__ == '__main__':
  sys.exit(main())
