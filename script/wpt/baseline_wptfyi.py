"""Fetch the newest complete Chrome run from wpt.fyi as a baseline.

Uses the wpt.fyi API documented at
https://github.com/web-platform-tests/wpt.fyi/blob/main/api/README.md:
`/api/runs` lists test runs, newest first, and each run has a
`raw_results_url` pointing at the wptreport JSON that was uploaded for it.

Runs are picked by label. wpt.fyi keeps the channel a run was uploaded with
as a label, so Chrome Canary runs carry `canary` (and `experimental`, which
Chrome Dev runs carry too); see `_channel_to_labels` in
https://github.com/web-platform-tests/wpt.fyi/blob/main/results-processor/wptreport.py.
"""

import gzip
import json
import urllib.parse
import urllib.request

API = 'https://wpt.fyi/api/runs'
# A full run has about 60,000 tests of the types that are compared; anything
# much smaller is a partial upload.
MIN_TESTS = 30000


def fetch(url, timeout=600):
  request = urllib.request.Request(url, headers={'User-Agent': 'electron-wpt'})
  with urllib.request.urlopen(request, timeout=timeout) as response:
    data = response.read()
  if data[:2] == b'\x1f\x8b':
    data = gzip.decompress(data)
  return data


def list_runs(label='canary', max_count=20):
  """Chrome runs of upstream wpt's master branch on Linux, newest first."""
  query = urllib.parse.urlencode([('product', 'chrome'), ('label', 'master'),
                                  ('label', label), ('max-count', max_count)])
  runs = json.loads(fetch(f'{API}?{query}', timeout=60))
  return [
      run for run in runs if run.get('os_name') == 'linux'
      and run.get('raw_results_url') and run.get('full_revision_hash')
  ]


def to_baseline(report):
  """Converts a wptreport dict to the baseline format of diff.py: the status
  Chrome got is the only one the baseline accepts."""
  tests = {}
  for result in report['results']:
    entry = {}
    if result['status'] not in ('PASS', 'OK'):
      entry['expected'] = [result['status']]
    subtests = {
        s['name']: [s['status']]
        for s in result.get('subtests', ()) if s['status'] != 'PASS'
    }
    if subtests:
      entry['subtests'] = subtests
    tests[result['test']] = entry
  return tests


def newest_complete_run(label='canary', log=print):
  """Returns (run, baseline tests) for the newest run that is complete."""
  for run in list_runs(label):
    log(f"Fetching {run['raw_results_url']}")
    try:
      report = json.loads(fetch(run['raw_results_url']))
    except (OSError, ValueError) as e:
      log(f'  skipped: {e}')
      continue
    if len(report.get('results', ())) < MIN_TESTS:
      log(f"  skipped: only {len(report.get('results', ()))} tests")
      continue
    return run, to_baseline(report)
  raise RuntimeError(
      f'wpt.fyi has no complete Chrome ({label}) run of master on Linux')
