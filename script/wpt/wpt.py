#!/usr/bin/env python3
"""Run web-platform-tests against Electron and compare with Chromium.

  wpt.py prepare    fetch the tests and the baseline to compare with
  wpt.py run        run the tests (or one chunk of them) on Electron
  wpt.py summarize  compare the reports with the baseline
  wpt.py mojojs     package the MojoJS bindings of an Electron build

See README.md in this directory.
"""

import argparse
import glob
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ELECTRON_ROOT = os.path.dirname(os.path.dirname(HERE))

sys.path.insert(0, HERE)
import baseline_wptfyi  # noqa: E402
import diff  # noqa: E402

WPT_GIT = 'https://github.com/web-platform-tests/wpt.git'
CFT_URL = ('https://storage.googleapis.com/chrome-for-testing-public/'
           '%s/mojojs.zip')

# What `pinned` takes from the Chromium checkout Electron is built from: the
# tests, the expectations, Chromium's wptrunner, and blinkpy with the files it
# imports on the way. Paths ending in /* stand for the files directly in that
# directory.
CHROMIUM_PATHS = [
    'DEPS',
    'vpython.toml',
    'vpython.toml.uv.lock',
    'build/util',
    'testing',
    'third_party/blink/tools',
    'third_party/blink/web_tests/*',
    'third_party/blink/web_tests/TestLists',
    'third_party/blink/web_tests/external',
    'third_party/blink/web_tests/platform/linux/external/wpt',
    'third_party/blink/web_tests/platform/win/external/wpt',
    'third_party/blink/web_tests/resources',
    # blinkpy parses TestExpectations with typ.
    'third_party/catapult/third_party/typ',
    'third_party/wpt_tools',
]

# wdspec and print-reftest need CDP commands Electron does not implement yet
# (see expectations/exclude.txt).
TEST_TYPES = ['testharness', 'reftest', 'crashtest']

# Browser switches for `pinned`, on top of the ones baseline_chromium.py reads
# out of blinkpy.
CHROMIUM_BINARY_ARGS = [
    # From WPTAdapter._set_up_runner_config_options.
    '--host-resolver-rules=MAP nonexistent.*.test ^NOTFOUND,'
    'MAP *.test 127.0.0.1, MAP *.test. 127.0.0.1',
    # run_wpt_tests.py leaves --enable-experimental unset, which makes
    # wptrunner's chrome product pass both of these.
    '--enable-experimental-web-platform-features',
    '--enable-blink-test-features',
    # headless_shell is given --accept-lang=en-US,en, which Electron does not
    # have; this is the closest equivalent.
    '--lang=en-US',
]

# Test statuses the retry pass does not run again: every timeout would take
# the whole (multiplied) test timeout a second time. Such a test counts as
# failing after its one run.
NO_RETRY = ('TIMEOUT', 'EXTERNAL-TIMEOUT')

WPT_HOSTS = ('web-platform.test,.web-platform.test,'
             'not-web-platform.test,.not-web-platform.test')


def log(message):
  print(f'[wpt] {message}', file=sys.stderr, flush=True)


def run(cmd):
  """Runs a command, keeping its output off stdout (see cmd_hosts)."""
  log(f"$ {' '.join(cmd)}")
  return subprocess.run(cmd, check=True, stdout=sys.stderr)


def read_json(path):
  with open(path, encoding='utf-8') as f:
    return json.load(f)


def write_json(path, data):
  with open(path, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=1)


def deps_var(deps_path, name):
  with open(deps_path, encoding='utf-8') as f:
    match = re.search(rf"'{re.escape(name)}':\s*'([^']+)'", f.read())
  if not match:
    raise RuntimeError(f'{name} not found in {deps_path}')
  return match.group(1)


def clone_commit(url, revision, dest):
  """Fetches one commit without history."""
  if os.path.isdir(dest):
    log(f'Reusing {dest}')
    return
  os.makedirs(dest)

  def git(*args):
    run(['git', '-C', dest] + list(args))

  git('init', '-q')
  git('remote', 'add', 'origin', url)
  git('fetch', '-q', '--depth=1', 'origin', revision)
  git('-c', 'advice.detachedHead=false', 'checkout', '-q', 'FETCH_HEAD')
  # The history is not needed again and would double the size of the
  # artifact the tree is passed to the shards in.
  shutil.rmtree(os.path.join(dest, '.git'))


def copy_chromium_files(chromium_src, dest):
  """Copies CHROMIUM_PATHS out of a Chromium checkout, so that the shards get
  a tree of a few hundred MB instead of a whole checkout."""
  if os.path.isdir(dest):
    log(f'Reusing {dest}')
    return
  log(f'Copying Chromium\'s tests and tools from {chromium_src}')
  ignore = shutil.ignore_patterns('.git', '__pycache__')
  for path in CHROMIUM_PATHS:
    files_only = path.endswith('/*')
    path = path[:-2] if files_only else path
    source = os.path.join(chromium_src, *path.split('/'))
    target = os.path.join(dest, *path.split('/'))
    if not os.path.exists(source):
      raise SystemExit(f'{source} not found: --chromium-src must be the '
                       'Chromium checkout (src) that Electron is built in')
    if files_only:
      os.makedirs(target, exist_ok=True)
      for entry in os.scandir(source):
        if entry.is_file():
          shutil.copy2(entry.path, target)
    elif os.path.isdir(source):
      shutil.copytree(source, target, symlinks=True, ignore=ignore)
    else:
      os.makedirs(os.path.dirname(target), exist_ok=True)
      shutil.copy2(source, target)


def chromium_checkout_version(chromium_src):
  try:
    with open(os.path.join(chromium_src, 'chrome', 'VERSION'),
              encoding='utf-8') as f:
      parts = dict(line.strip().split('=', 1) for line in f if '=' in line)
    return '.'.join(parts[key] for key in ('MAJOR', 'MINOR', 'BUILD', 'PATCH'))
  except (OSError, KeyError):
    return None


def paths_for(work, revision=None):
  """Where things are in a work directory."""
  p = {
      'meta': os.path.join(work, 'meta.json'),
      'baseline': os.path.join(work, 'baseline.json'),
      'venv': os.path.join(work, 'venv'),
      'mojojs': os.path.join(work, 'mojojs')
  }
  if (revision or read_json(p['meta'])['revision']) == 'pinned':
    src = os.path.join(work, 'chromium', 'src')
    p['chromium'] = src
    p['vpython_spec'] = os.path.join(src, 'vpython.toml')
    p['wpt_tools'] = os.path.join(src, 'third_party', 'wpt_tools', 'wpt')
    p['tests'] = os.path.join(src, 'third_party', 'blink', 'web_tests',
                              'external', 'wpt')
    p['certs'] = os.path.join(src, 'third_party', 'wpt_tools', 'certs')
    p['skip'] = os.path.join(work, 'chromium-skip.txt')
  else:
    p['wpt_tools'] = p['tests'] = os.path.join(work, 'wpt')
  return p


def python_command(work, p):
  """The command prefix that starts the Python the wpt tools run with.

  `pinned` uses depot_tools' vpython3 with Chromium's own vpython.toml, which
  is how Chromium runs blinkpy and wptrunner: the same Python version and the
  same packages, with nothing to install here. `latest` runs upstream wpt,
  which has no vpython spec, in a virtualenv with wpt's requirements."""
  if 'vpython_spec' in p:
    return ensure_vpython(work, p['vpython_spec'])
  return [ensure_venv(work, p['wpt_tools'])]


_VPYTHON = []


def ensure_vpython(work, spec):
  if _VPYTHON:
    return list(_VPYTHON)
  vpython3 = shutil.which('vpython3')
  if not vpython3:
    raise SystemExit('vpython3 not found: `pinned` runs Chromium\'s Python '
                     'tools with vpython3, so depot_tools has to be on PATH')
  # vpython keeps its environments in the user's cache directory. CI runs
  # this as root and then as builduser, whose home directory may not be
  # writable, so keep them in the work directory, which both can write to.
  # They are set in os.environ because wptrunner starts more Python
  # processes, which go through vpython again.
  os.environ.setdefault('VPYTHON_VIRTUALENV_ROOT',
                        os.path.join(work, 'vpython-root'))
  if not os.access(os.path.expanduser('~'), os.W_OK):
    os.environ.setdefault('XDG_CACHE_HOME', os.path.join(work, 'cache'))
  # No `--` after vpython's own flags: vpython hands it on to Python, which
  # then takes whatever follows as a script path, `-c` and `-m` included.
  command = [vpython3, '-vpython-spec', spec]
  # The first use downloads vpython itself and then Python and the packages
  # in the spec. Do that here, where a failure is easy to tell apart from a
  # failure of the tool that was going to run, and try again if it fails.
  probe = command + ['-c', 'import sys; print(sys.version)']
  log(f"$ {' '.join(probe)}")
  for attempt in range(3):
    if attempt:
      time.sleep(10 * attempt)
    result = subprocess.run(probe,
                            check=False,
                            stdout=subprocess.PIPE,
                            text=True)
    if result.returncode == 0:
      log(f'vpython3 gives Python {result.stdout.strip()}')
      _VPYTHON.extend(command)
      return command
    log(f'vpython3 exited with {result.returncode}')
  raise SystemExit(f'vpython3 could not set up the environment in {spec}')


def ensure_venv(work, wpt_tools):
  """Creates the virtualenv wptrunner runs in. It is not relocatable, so
  every job creates its own instead of receiving it with the tree."""
  python = os.path.join(work, 'venv', 'bin', 'python')
  marker = os.path.join(work, 'venv', '.electron-wpt')
  if os.path.exists(marker):
    return python
  run([sys.executable, '-m', 'venv', os.path.join(work, 'venv')])
  pip = [
      python, '-m', 'pip', 'install', '--quiet', '--disable-pip-version-check'
  ]
  # What `wpt run` installs.
  for name in ('manifest', 'wptrunner', 'serve'):
    run(pip +
        ['-r', os.path.join(wpt_tools, 'tools', name, 'requirements.txt')])
  # WebTransport needs aioquic, which does not build everywhere; without it
  # the WebTransport-over-HTTP/3 server is left off.
  path = os.path.join(wpt_tools, 'tools', 'webtransport', 'requirements.txt')
  if subprocess.run(pip + ['-r', path], check=False,
                    stdout=sys.stderr).returncode != 0:
    log('WebTransport requirements failed to install; continuing without')
  with open(marker, 'w', encoding='utf-8'):
    pass
  return python


def wpt_cli(work, p):
  """The `wpt` command line of the tools in the work directory, through
  wpt_cli.py so that it knows the `electron` product. --skip-venv-setup keeps
  it from creating a virtualenv of its own and installing into it."""
  return python_command(work, p) + [
      os.path.join(HERE, 'wpt_cli.py'), p['wpt_tools'], '--venv', p['venv'],
      '--skip-venv-setup'
  ]


def fetch_mojojs(version, dest):
  """MojoJS bindings (for WebUSB, WebHID and similar tests) are published
  with Chrome for Testing, but not for every Chromium version."""
  if os.path.isdir(dest):
    return True
  archive = f"{dest}.zip"
  try:
    urllib.request.urlretrieve(CFT_URL % version, archive)
  except (urllib.error.URLError, OSError) as e:
    log(f'No MojoJS bindings for {version} ({e}); '
        'tests that need them will fail')
    return False
  extracted = f"{dest}.tmp"
  run(['unzip', '-q', '-o', archive, '-d', extracted])
  os.remove(archive)
  # --mojojs-path is the directory served as /gen/; the archive has it
  # inside one top-level directory.
  roots = glob.glob(os.path.join(extracted, '*', 'gen')) + glob.glob(
      os.path.join(extracted, 'gen'))
  if not roots:
    log('Unexpected mojojs.zip layout; continuing without MojoJS')
    shutil.rmtree(extracted)
    return False
  shutil.move(roots[0], dest)
  shutil.rmtree(extracted, ignore_errors=True)
  return True


# What of a build's gen/ directory the tests load from /gen/: the ES module
# bindings of every mojom, and the bindings libraries.
MOJOJS_PATTERNS = [
    '**/*.mojom.m.js',
    'mojo/public/js/*.js',
    'layout_test_data/mojo/public/js/*.js',
]

# In mojojs.zip so that a build without the bindings target (the
# electron_wpt_mojojs group in BUILD.gn) fails here and not as test failures.
MOJOJS_REQUIRED = [
    'mojo/public/js/bindings.js',
    'layout_test_data/mojo/public/js/mojo_bindings.js',
    'device/vr/public/mojom/vr_service.mojom.m.js',
    'services/shape_detection/public/mojom/facedetection.mojom.m.js',
]


def cmd_mojojs(args):
  """Writes mojojs.zip, like the one published with Chrome for Testing but
  from an Electron build: the bindings have to be from the build that is
  tested, since mojom message IDs differ between builds (official builds
  scramble them)."""
  gen = os.path.abspath(args.gen_dir)
  missing = [
      name for name in MOJOJS_REQUIRED
      if not os.path.isfile(os.path.join(gen, name))
  ]
  if missing:
    raise SystemExit(f"Not in {gen}: {', '.join(missing)}. "
                     'Build //electron:electron_wpt_mojojs first.')
  files = sorted({
      path
      for pattern in MOJOJS_PATTERNS
      for path in glob.glob(os.path.join(gen, pattern), recursive=True)
  })
  with zipfile.ZipFile(args.out, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in files:
      archive.write(path, os.path.join('gen', os.path.relpath(path, gen)))
  log(f'Wrote {len(files)} files to {args.out}')


def prepare_pinned(work, meta, chromium_src):
  version = deps_var(os.path.join(ELECTRON_ROOT, 'DEPS'), 'chromium_version')
  meta['chromium_version'] = version
  p = paths_for(work, 'pinned')
  src = p['chromium']
  if not os.path.isdir(src):
    checkout = chromium_checkout_version(chromium_src)
    if checkout and checkout != version:
      log(f'Warning: {chromium_src} is at Chromium {checkout} but DEPS has '
          f'{version}; the tests and the baseline are from {checkout}')
  copy_chromium_files(chromium_src, src)
  python = python_command(work, p)

  binary_args_path = os.path.join(work, 'chromium-binary-args.json')
  run(python + [
      os.path.join(HERE, 'baseline_chromium.py'), '--chromium-src', src,
      '--out', p['baseline'], '--skip-out', p['skip'], '--runner-args-out',
      binary_args_path
  ])
  meta['binary_args'] = read_json(binary_args_path) + CHROMIUM_BINARY_ARGS
  os.remove(binary_args_path)

  meta['mojojs'] = fetch_mojojs(version, p['mojojs'])
  meta['baseline'] = f'Chromium {version}'
  meta['tests_from'] = (f'Chromium {version}, '
                        '`third_party/blink/web_tests/external/wpt`')
  meta['baseline_detail'] = ('Chromium\'s in-tree `TestExpectations` and '
                             f'`-expected.txt` baselines at {version}')


def write_chromium_server_config(p):
  """What WPTAdapter.create_config_files does: writes the wpt server
  configuration with the alias directories made absolute. Done in `run`
  because the tree is moved between jobs."""
  web_tests = os.path.dirname(os.path.dirname(p['tests']))
  config = read_json(os.path.join(p['tests'], 'config.tmpl.json'))
  aliases = []
  for alias in config['aliases']:
    local_dir = os.path.normpath(os.path.join(web_tests, alias['local-dir']))
    # wpt_internal is not checked out: only external/wpt is run.
    if os.path.isdir(local_dir):
      aliases.append(dict(alias, **{'local-dir': local_dir}))
  config['aliases'] = aliases
  write_json(os.path.join(p['tests'], 'config.json'), config)


def prepare_latest(work, meta, label):
  run_info, tests = baseline_wptfyi.newest_complete_run(label, log)
  sha = run_info['full_revision_hash']
  write_json(os.path.join(work, 'baseline.json'), {'tests': tests})
  del tests
  p = paths_for(work, 'latest')
  wpt = p['tests']
  clone_commit(WPT_GIT, sha, wpt)
  run(
      wpt_cli(work, p) + [
          'manifest', '--no-download', '--tests-root', wpt, '--path',
          os.path.join(wpt, 'MANIFEST.json')
      ])
  meta['wpt_sha'] = sha
  meta['wptfyi_run'] = {
      k: run_info.get(k)
      for k in ('id', 'browser_name', 'browser_version', 'os_name',
                'os_version', 'revision', 'full_revision_hash', 'time_start',
                'raw_results_url', 'labels')
  }
  meta['wptfyi_label'] = label
  meta['binary_args'] = []
  meta['mojojs'] = False
  version = run_info.get('browser_version', label)
  run_id = run_info.get('id')
  meta['baseline'] = f'Chrome {version} on wpt.fyi'
  meta['tests_from'] = f'web-platform-tests/wpt@{sha[:12]}'
  meta['baseline_detail'] = (
      f"{run_info.get('browser_name', 'chrome')} {version} ({label}), "
      f'[wpt.fyi run {run_id}](https://wpt.fyi/results/?run_id={run_id})')


def cmd_prepare(args):
  work = os.path.abspath(args.work)
  os.makedirs(work, exist_ok=True)
  meta = {'revision': args.revision}
  if args.revision == 'pinned':
    prepare_pinned(work, meta, os.path.abspath(args.chromium_src))
  else:
    prepare_latest(work, meta, args.wptfyi_label)
  write_json(os.path.join(work, 'meta.json'), meta)
  # Every job makes its own Python environment (see python_command).
  if not args.keep_venv:
    for name in ('venv', 'vpython-root', 'cache'):
      shutil.rmtree(os.path.join(work, name), ignore_errors=True)
  log(f"Prepared {work}: tests from {meta['tests_from']}, "
      f"baseline {meta['baseline']}")


def has_ipv6():
  try:
    with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as s:
      s.bind(('::1', 0))
    return True
  except OSError:
    return False


def find_binary(directory, name):
  for candidate in (os.path.join(directory, name),
                    os.path.join(directory, f'{name}-linux64', name)):
    if os.path.isfile(candidate):
      return os.path.abspath(candidate)
  raise SystemExit(f'{name} not found in {directory}')


def runner_command(args, work, p, meta, report, extra):
  electron = find_binary(args.electron_dir, 'electron')
  chromedriver = find_binary(args.chromedriver_dir or args.electron_dir,
                             'chromedriver')
  manifest = os.path.join(p['tests'], 'MANIFEST.json')
  cmd = wpt_cli(work, p) + [
      'run',
      'electron',
      '--yes',
      f'--binary={electron}',
      f'--webdriver-binary={chromedriver}',
      f"--electron-app={os.path.join(HERE, 'shell')}",
      '--electron-xvfb',
      f"--tests={p['tests']}",
      f"--metadata={os.path.join(work, 'metadata')}",
      f'--manifest={manifest}',
      '--no-manifest-update',
      '--test-types',
      *TEST_TYPES,
      f'--processes={args.processes}',
      f'--timeout-multiplier={args.timeout_multiplier}',
      f"--exclude-file={os.path.join(work, 'exclude.txt')}",
      # Collect results like wpt.fyi and Chromium's bots do: nothing is
      # "unexpected" to wptrunner, the comparison happens afterwards.
      '--no-fail-on-unexpected',
      '--no-restart-on-unexpected',
      '--no-pause-after-test',
      f'--log-wptreport={report}',
      '--log-mach=-',
      '--log-mach-level=info',
  ]
  if meta['revision'] == 'pinned':
    # The rest of what blinkpy's WPTAdapter sets up for wptrunner.
    certs = p['certs']
    cmd += [
        f"--ca-cert-path={os.path.join(certs, 'cacert.pem')}",
        f"--host-key-path={os.path.join(certs, '127.0.0.1.key')}",
        f"--host-cert-path={os.path.join(certs, '127.0.0.1.pem')}",
        '--no-restart-on-new-group',
        '--run-by-dir=0',
        '--reuse-window',
    ]
  else:
    # Match how upstream wpt's Taskcluster jobs produced the Chrome run that
    # is compared with (tools/ci/tc/tasks/test.yml, taskcluster-run.py):
    # `wpt run --channel=canary` or `dev` turns on experimental web platform
    # features, `beta` and `stable` leave them off, and every Chrome run gets
    # --enable-swiftshader.
    experimental = meta.get('wptfyi_label') in ('canary', 'dev', 'experimental')
    cmd += [
        '--enable-experimental' if experimental else '--no-enable-experimental',
        '--enable-swiftshader',
    ]
  # The bindings of the Electron build when they are given, else the ones
  # `prepare` got from Chrome for Testing (see README.md for the difference).
  mojojs = None
  if args.mojojs:
    mojojs = os.path.abspath(args.mojojs)
    if not os.path.isfile(os.path.join(mojojs, 'mojo', 'public', 'js',
                                       'bindings.js')):
      raise SystemExit(f'{mojojs} is not the gen directory of a build with '
                       'MojoJS bindings')
  elif meta.get('mojojs'):
    mojojs = p['mojojs']
  if mojojs:
    cmd += ['--enable-mojojs', f'--mojojs-path={mojojs}']
  binary_args = list(meta.get('binary_args', []))
  if os.geteuid() == 0:
    binary_args.append('--no-sandbox')
  cmd += [f"--binary-arg={arg}" for arg in binary_args]
  if args.strip_feature_params:
    cmd.append('--electron-strip-feature-params')
  if args.reftest_resize:
    cmd.append('--electron-reftest-resize')
  if not has_ipv6():
    cmd += [
        '--no-enable-webtransport-h3', '--electron-explicit-webdriver-port'
    ]
  else:
    try:
      subprocess.run(python_command(work, p) + ['-c', 'import aioquic'],
                     check=True,
                     stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
      cmd.append('--no-enable-webtransport-h3')
    else:
      # `wpt run` starts the WebTransport-over-HTTP/3 server by itself only
      # for its built-in browsers (the setup classes in tools/wpt/run.py);
      # for any other product it has to be asked for.
      cmd.append('--enable-webtransport-h3')
  return cmd + extra


def run_wptrunner(cmd, report):
  env = dict(os.environ)
  # Keep the wpt hosts off any proxy. --no-proxy-server would do that too
  # but also disables the PAC script some tests configure.
  for name in ('no_proxy', 'NO_PROXY'):
    env[name] = ','.join(filter(None, [env.get(name), WPT_HOSTS]))
  env.pop('NODE_OPTIONS', None)
  log(f"$ {' '.join(cmd)}")
  code = subprocess.run(cmd, env=env, check=False).returncode
  try:
    results = len(read_json(report)['results'])
  except (OSError, ValueError, KeyError):
    results = 0
  log(f'wptrunner exited with {code}; {results} results in {report}')
  return results


def cmd_run(args):
  work = os.path.abspath(args.work)
  p = paths_for(work)
  meta = read_json(p['meta'])
  os.makedirs(os.path.join(work, 'metadata'), exist_ok=True)
  os.makedirs(args.out, exist_ok=True)
  if meta['revision'] == 'pinned':
    write_chromium_server_config(p)

  with open(os.path.join(work, 'exclude.txt'), 'w', encoding='utf-8') as out:
    for path in (os.path.join(HERE, 'expectations',
                              'exclude.txt'), p.get('skip')):
      if path and os.path.exists(path):
        with open(path, encoding='utf-8') as f:
          out.writelines(line for line in f
                         if line.strip() and not line.startswith('#'))

  name = 'wptreport'
  extra = []
  if args.total_chunks > 1:
    name += f'-{args.this_chunk}'
    # id_hash spreads tests evenly; the default (dir_hash) keeps
    # directories together, and their sizes vary a lot.
    extra = [
        f'--total-chunks={args.total_chunks}',
        f'--this-chunk={args.this_chunk}',
        '--chunk-type=id_hash',
    ]
  report = os.path.abspath(os.path.join(args.out, f"{name}.json"))
  if not run_wptrunner(
      runner_command(args, work, p, meta, report, extra + args.tests), report):
    raise SystemExit('wptrunner produced no results')

  # Run what failed once more, so that the summary can tell flaky tests from
  # failing ones (see NO_RETRY for the exception).
  known = diff.Known(os.path.join(HERE, 'expectations',
                                  'known-differences.txt'))
  result = diff.compare(read_json(p['baseline']), [read_json(report)], known)
  retry = sorted({
      item['test']
      for item in result['new']
      if item['subtest'] is not None or item['status'] not in NO_RETRY
  })
  log(f"{len({item['test'] for item in result['new']})} tests have new "
      f'Electron-only failures, {len(retry)} to run again')
  if not retry or args.no_retry:
    return
  if len(retry) > args.retry_limit:
    log(f'Not retrying: more than {args.retry_limit} tests failed')
    return
  include = os.path.join(work, 'retry.txt')
  with open(include, 'w', encoding='utf-8') as f:
    f.writelines(f"{test}\n" for test in retry)
  retry_report = os.path.abspath(os.path.join(args.out, f"{name}-retry.json"))
  run_wptrunner(
      runner_command(args, work, p, meta, retry_report,
                     [f'--include-file={include}']), retry_report)


def cmd_summarize(args):
  work = os.path.abspath(args.work)
  meta = read_json(os.path.join(work, 'meta.json'))
  baseline = read_json(os.path.join(work, 'baseline.json'))
  paths = sorted(p for pattern in args.reports for p in glob.glob(
      os.path.join(pattern, '**', 'wptreport*.json'), recursive=True))
  # First runs before retries, so that the first result of a test is the
  # one that is counted.
  paths.sort(key=lambda path: path.endswith('-retry.json'))
  reports = [read_json(path) for path in paths]
  first_runs = [path for path in paths if not path.endswith('-retry.json')]

  problems = []
  if args.expect_reports and len(first_runs) != args.expect_reports:
    problems.append(f'Expected {args.expect_reports} reports but found '
                    f'{len(first_runs)}; some shards did not finish.')
  if not reports:
    problems.append('No reports found.')
  for report in reports[:1]:
    info = report.get('run_info', {})
    electron = info.get('electron_version', 'unknown')
    chromium = info.get('browser_version', 'unknown')
    meta['electron'] = f'{electron} (Chromium {chromium})'

  known = diff.Known(os.path.join(HERE, 'expectations',
                                  'known-differences.txt'))
  result = diff.compare(baseline, reports, known)
  summary = diff.markdown(result, meta, problems)
  if args.summary:
    with open(args.summary, 'a', encoding='utf-8') as f:
      f.write(summary)
  else:
    print(summary)
  if args.out:
    os.makedirs(args.out, exist_ok=True)
    write_json(os.path.join(args.out, 'wpt-diff.json'), dict(result, meta=meta))
    with open(os.path.join(args.out, 'new-failures.txt'), 'w',
              encoding='utf-8') as f:
      f.writelines(f"{line}\n"
                   for line in diff.known_differences_lines(result['new']))
  counts = result['counts']
  log(f"{counts.get('tests', 0)} tests run, {len(result['new'])} new "
      f"Electron-only failures, {len(result['known'])} known, "
      f"{len(result['flaky'])} flaky")
  if problems or result['new']:
    raise SystemExit(1)


HOSTS_SCRIPT = """
import logging, sys
sys.path.insert(0, sys.argv[1])
from tools import localpaths  # noqa: F401
from tools.serve import serve
with serve.build_config(logging.getLogger(), ssl={"type": "none"}) as config:
  print(serve.make_hosts_file(config, "127.0.0.1"))
"""


def cmd_hosts(args):
  """Prints the /etc/hosts lines the wpt servers need. This is what
  `wpt make-hosts-file` does; Chromium's copy of the tools lacks it."""
  work = os.path.abspath(args.work)
  p = paths_for(work)
  subprocess.run(python_command(work, p) +
                 ['-c', HOSTS_SCRIPT, p['wpt_tools']],
                 check=True)


def main():
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
  sub = parser.add_subparsers(dest='command', required=True)

  prepare = sub.add_parser('prepare', help='fetch the tests and the baseline')
  prepare.add_argument('--work', required=True, help='directory to create')
  prepare.add_argument(
      '--revision',
      choices=['pinned', 'latest'],
      default='pinned',
      help=
      'pinned: the tests and expectations in Chromium at the version in DEPS. '
      'latest: the newest upstream wpt commit with a complete Chrome Canary '
      'run on wpt.fyi, and that run\'s results.')
  prepare.add_argument(
      '--wptfyi-label',
      default='canary',
      help='wpt.fyi label of the Chrome run to compare with: canary, dev, '
      'beta or stable'
  )
  prepare.add_argument(
      '--chromium-src',
      default=os.path.dirname(ELECTRON_ROOT),
      help='pinned: the Chromium checkout to take the tests, the '
      'expectations and the tools from (default: the one this Electron '
      'checkout is in)')
  prepare.add_argument(
      '--keep-venv',
      action='store_true',
      help='keep the Python environment (when `run` follows in the same '
      'place)')
  prepare.set_defaults(func=cmd_prepare)

  run_parser = sub.add_parser('run', help='run the tests on Electron')
  run_parser.add_argument('--work',
                          required=True,
                          help='directory made by `prepare`')
  run_parser.add_argument('--electron-dir',
                          required=True,
                          help='directory with the electron binary')
  run_parser.add_argument(
      '--chromedriver-dir',
      help='directory with chromedriver (default: --electron-dir)')
  run_parser.add_argument(
      '--mojojs',
      help='directory to serve as /gen/, with the MojoJS bindings of the '
      'Electron build: its out/<config>/gen, or gen/ from the archive '
      '`mojojs` writes (default: the bindings of Chrome for Testing)')
  run_parser.add_argument('--out',
                          required=True,
                          help='directory to write wptreport files to')
  run_parser.add_argument('--processes', type=int, default=os.cpu_count() or 2)
  run_parser.add_argument('--timeout-multiplier', type=float, default=1)
  run_parser.add_argument('--total-chunks', type=int, default=1)
  run_parser.add_argument('--this-chunk', type=int, default=1)
  run_parser.add_argument(
      '--strip-feature-params',
      action='store_true',
      help='needed for builds with DCHECKs (electron/electron#55990)')
  run_parser.add_argument(
      '--reftest-resize',
      action='store_true',
      help='resize the window for reftests (needs electron/electron#55945)')
  run_parser.add_argument('--no-retry', action='store_true')
  run_parser.add_argument('--retry-limit',
                          type=int,
                          default=300,
                          help='do not retry when more tests than this failed')
  run_parser.add_argument('tests',
                          nargs='*',
                          help='test paths (default: everything)')
  run_parser.set_defaults(func=cmd_run)

  summarize = sub.add_parser('summarize',
                             help='compare reports with the baseline')
  summarize.add_argument('--work',
                         required=True,
                         help='directory made by `prepare`')
  summarize.add_argument('--reports',
                         nargs='+',
                         required=True,
                         help='directories to look for wptreport*.json in')
  summarize.add_argument(
      '--expect-reports',
      type=int,
      default=0,
      help='fail unless this many first-run reports are found')
  summarize.add_argument(
      '--summary',
      help='file to append the Markdown summary to (default: stdout)')
  summarize.add_argument(
      '--out', help='directory to write wpt-diff.json and new-failures.txt to')
  summarize.set_defaults(func=cmd_summarize)

  hosts = sub.add_parser('hosts',
                         help='print the /etc/hosts lines the wpt servers need')
  hosts.add_argument('--work', required=True)
  hosts.set_defaults(func=cmd_hosts)

  mojojs = sub.add_parser(
      'mojojs', help='package the MojoJS bindings of an Electron build')
  mojojs.add_argument('--gen-dir',
                      required=True,
                      help='gen directory of the build, after building '
                      '//electron:electron_wpt_mojojs')
  mojojs.add_argument('--out', required=True, help='archive to write')
  mojojs.set_defaults(func=cmd_mojojs)

  args = parser.parse_args()
  args.func(args)


if __name__ == '__main__':
  main()
