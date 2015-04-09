#!/usr/bin/env python
# Copyright (c) 2013 GitHub, Inc.
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Convert pdb to sym for given directories"""

import errno
import glob
import optparse
import os
import Queue
import re
import subprocess
import sys
import threading


CONCURRENT_TASKS=4
SOURCE_ROOT=os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
DUMP_SYMS=os.path.join(SOURCE_ROOT, 'vendor', 'breakpad', 'dump_syms.exe')


def GetCommandOutput(command):
  """Runs the command list, returning its output.

  Prints the given command (which should be a list of one or more strings),
  then runs it and returns its output (stdout) as a string.

  From chromium_utils.
  """
  devnull = open(os.devnull, 'w')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=devnull,
                          bufsize=1)
  output = proc.communicate()[0]
  return output


def mkdir_p(path):
  """Simulates mkdir -p."""
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else: raise


def GenerateSymbols(options, binaries):
  """Dumps the symbols of binary and places them in the given directory."""

  queue = Queue.Queue()
  print_lock = threading.Lock()

  def _Worker():
    while True:
      binary = queue.get()

      if options.verbose:
        with print_lock:
          print "Generating symbols for %s" % binary

      syms = GetCommandOutput([DUMP_SYMS, binary])
      module_line = re.match("MODULE [^ ]+ [^ ]+ ([0-9A-Fa-f]+) (.*)\r\n", syms)
      if module_line == None:
        with print_lock:
          print "Failed to get symbols for %s" % binary
        queue.task_done()
        continue

      output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                 module_line.group(1))
      mkdir_p(output_path)
      symbol_file = "%s.sym" % module_line.group(2)[:-4]  # strip .pdb
      f = open(os.path.join(output_path, symbol_file), 'w')
      f.write(syms)
      f.close()

      queue.task_done()

  for binary in binaries:
    queue.put(binary)

  for _ in range(options.jobs):
    t = threading.Thread(target=_Worker)
    t.daemon = True
    t.start()

  queue.join()


def main():
  parser = optparse.OptionParser()
  parser.add_option('', '--symbols-dir', default='',
                    help='The directory where to write the symbols file.')
  parser.add_option('', '--clear', default=False, action='store_true',
                    help='Clear the symbols directory before writing new '
                         'symbols.')
  parser.add_option('-j', '--jobs', default=CONCURRENT_TASKS, action='store',
                    type='int', help='Number of parallel tasks to run.')
  parser.add_option('-v', '--verbose', action='store_true',
                    help='Print verbose status output.')

  (options, directories) = parser.parse_args()

  if not options.symbols_dir:
    print "Required option --symbols-dir missing."
    return 1

  if options.clear:
    try:
      shutil.rmtree(options.symbols_dir)
    except:
      pass

  pdbs = []
  for directory in directories:
    pdbs += glob.glob(os.path.join(directory, '*.exe.pdb'))
    pdbs += glob.glob(os.path.join(directory, '*.dll.pdb'))

  GenerateSymbols(options, pdbs)

  return 0


if '__main__' == __name__:
  sys.exit(main())
