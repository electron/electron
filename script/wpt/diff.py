"""Compare Electron wptreport files with a baseline of what Chromium does.

Baseline file (written by baseline_chromium.py and baseline_wptfyi.py):

  {"source": {...},
   "tests": {"/dom/example.html": {
     "expected": ["OK", "TIMEOUT"],      # statuses the baseline accepts
     "any_subtests": true,               # subtest results are not checked
     "subtests": {"name": ["FAIL"]}}}}   # subtests that are not plain passes

Every key is optional: a test with an empty entry passes with all its
subtests, and a test with no entry at all is not in the baseline and is not
compared.

A result is an Electron-only failure when Electron does not pass, the baseline
accepts a pass, and the baseline does not accept the status Electron got. When
a test is run more than once (the shards retry what failed), it only counts if
every run fails; results that differ between runs are reported as flaky.

Known differences (expectations/known-differences.txt) are lines of up to
three `|`-separated glob patterns: test URL, subtest name, message. Missing
patterns match everything. Blank lines and lines starting with # are ignored.
"""

import collections
import fnmatch
import json
import re

PASS = ('PASS', 'OK')
# With these test statuses the subtest results are an arbitrary prefix of the
# real ones, so they are not compared (Chromium does the same).
INCOMPLETE = ('TIMEOUT', 'CRASH', 'EXTERNAL-TIMEOUT')


def load_json(path):
  with open(path, encoding='utf-8') as f:
    return json.load(f)


class Known:
  """The known-differences list."""

  def __init__(self, path=None):
    self.rules = []
    if path:
      with open(path, encoding='utf-8') as f:
        for number, line in enumerate(f, 1):
          line = line.strip()
          if not line or line.startswith('#'):
            continue
          parts = [p.strip() for p in line.split(' | ')]
          parts += ['*'] * (3 - len(parts))
          self.rules.append({
              'line':
              number,
              'text':
              line,
              'regexes':
              [re.compile(fnmatch.translate(p), re.DOTALL) for p in parts[:3]],
              'hits':
              0,
          })

  def match(self, test, subtest, message):
    for rule in self.rules:
      test_re, subtest_re, message_re = rule['regexes']
      if (test_re.match(test) and subtest_re.match((subtest or '').strip())
          and message_re.match(message or '')):
        rule['hits'] += 1
        return rule
    return None


def is_new_failure(status, expected):
  return (status not in PASS and status != 'SKIP' and status not in expected
          and any(s in PASS for s in expected))


def failures_in_report(report, baseline_tests):
  """Yields (test, subtest or None, status, message) for one report."""
  for result in report['results']:
    test = result['test']
    entry = baseline_tests.get(test)
    if entry is None:
      continue
    status = result['status']
    if is_new_failure(status, entry.get('expected', PASS)):
      yield test, None, status, result.get('message')
      continue
    if status in INCOMPLETE or entry.get('any_subtests'):
      continue
    expected_subtests = entry.get('subtests', {})
    for subtest in result['subtests']:
      if is_new_failure(subtest['status'],
                        expected_subtests.get(subtest['name'], PASS)):
        yield test, subtest['name'], subtest['status'], subtest.get('message')


def compare(baseline, reports, known=None):
  """Compares reports (parsed wptreport dicts, the first run of a test first)
  with the baseline. Returns a dict of counts and item lists."""
  known = known or Known()
  baseline_tests = baseline['tests']
  runs = collections.defaultdict(int)  # test -> number of reports with it
  failures = collections.defaultdict(
      list)  # (test, subtest) -> [(status, message)]
  counts = collections.Counter()
  seen = set()
  for report in reports:
    for result in report['results']:
      test = result['test']
      runs[test] += 1
      if test in seen:
        continue
      seen.add(test)
      if result['status'] == 'SKIP':
        counts['skipped'] += 1
        continue
      counts['tests'] += 1
      counts['tests_pass'] += result['status'] in PASS
      counts['subtests'] += len(result['subtests'])
      counts['subtests_pass'] += sum(s['status'] == 'PASS'
                                     for s in result['subtests'])
      if test not in baseline_tests:
        counts['not_in_baseline'] += 1
    for test, subtest, status, message in failures_in_report(
        report, baseline_tests):
      failures[(test, subtest)].append((status, message))

  new, known_items, flaky = [], [], []
  for (test, subtest), attempts in sorted(failures.items(),
                                          key=lambda i:
                                          (i[0][0], i[0][1] or '')):
    status, message = attempts[0]
    item = {
        'test': test,
        'subtest': subtest,
        'status': status,
        'message': (message or '').strip()[:500]
    }
    if len(attempts) < runs[test]:
      flaky.append(item)
      continue
    rule = known.match(test, subtest, message)
    if rule:
      item['known'] = rule['text']
      known_items.append(item)
    else:
      new.append(item)
  counts['baseline_tests'] = len(baseline_tests)
  counts['baseline_not_run'] = len(set(baseline_tests) - seen)
  return {
      'counts': dict(counts),
      'new': new,
      'known': known_items,
      'flaky': flaky,
      'unused_known': [r['text'] for r in known.rules if not r['hits']],
  }


def directory(test, depth=2):
  parts = test.split('?')[0].lstrip('/').split('/')
  return '/'.join(parts[:max(1, min(depth, len(parts) - 1))])


def _cell(text, limit=160):
  text = ' '.join((text or '').split())
  if len(text) > limit:
    text = f"{text[:limit - 1]}…"
  return text.replace('|', '\\|').replace('<', '&lt;')


def _items_table(items, limit):
  lines = ['| Test | Subtest | Electron | Message |', '|---|---|---|---|']
  for item in items[:limit]:
    lines.append(f"| `{item['test']}` | {_cell(item['subtest'], 100)} "
                 f"| {item['status']} | {_cell(item['message'])} |")
  if len(items) > limit:
    more = len(items) - limit
    lines.append(f'| … | {more} more in the `wpt-diff` artifact | | |')
  return lines


def markdown(result, meta, problems=(), list_limit=300):
  """Renders the comparison as GitHub-flavoured Markdown."""
  c = collections.Counter(result['counts'])
  new, known, flaky = result['new'], result['known'], result['flaky']
  title = meta.get('baseline', 'baseline')
  out = [f'## Web Platform Tests: Electron vs {title}', '']
  for key, label in (('electron', 'Electron'), ('tests_from', 'Tests'),
                     ('baseline_detail', 'Baseline')):
    if meta.get(key):
      out.append(f'- {label}: {meta[key]}')
  out.append('')
  for problem in problems:
    out.append('> [!WARNING]')
    out.append(f'> {problem}')
    out.append('')

  def pct(a, b):
    return f'{100.0 * a / b:.2f}%' if b else 'n/a'

  def split(items):
    tests = sum(1 for i in items if i['subtest'] is None)
    return f'{tests} tests, {len(items) - tests} subtests'

  out += [
      '| | |',
      '|---|---|',
      f"| Tests run | {c['tests']} ({c['baseline_not_run']} in the baseline "
      f"were not run, {c['not_in_baseline']} run are not in the baseline) |",
      f"| Tests passing on Electron | {c['tests_pass']} "
      f"({pct(c['tests_pass'], c['tests'])}) |",
      f"| Subtests passing on Electron | {c['subtests_pass']} of "
      f"{c['subtests']} ({pct(c['subtests_pass'], c['subtests'])}) |",
      f'| **New Electron-only failures** | **{split(new)}** |',
      f'| Known Electron-only failures | {split(known)} |',
      f'| Flaky (failed, then passed on retry) | {split(flaky)} |',
      '| Known-differences lines that matched nothing | '
      f"{len(result['unused_known'])} |",
      '',
  ]
  if new:
    out += [
        '### New Electron-only failures', '',
        'These pass in the baseline, fail on Electron and are not covered by '
        '`script/wpt/expectations/known-differences.txt`.', '',
        '| Directory | Failures |', '|---|---|'
    ]
    by_dir = collections.Counter(directory(i['test']) for i in new)
    out += [f'| `{name}` | {n} |' for name, n in by_dir.most_common(40)]
    if len(by_dir) > 40:
      out.append(f'| … | {len(by_dir) - 40} more directories |')
    out += ['', '<details><summary>List</summary>', ''] + _items_table(
        new, list_limit)
    out += ['', '</details>', '']
  else:
    out += ['No new Electron-only failures.', '']
  if known:
    out += [
        '<details><summary>Known Electron-only failures by rule</summary>', '',
        '| Rule | Failures |', '|---|---|'
    ]
    by_rule = collections.Counter(i['known'] for i in known)
    out += [
        f'| `{_cell(rule, 200)}` | {n} |' for rule, n in by_rule.most_common()
    ]
    out += ['', '</details>', '']
  if flaky:
    out += ['<details><summary>Flaky</summary>', ''] + _items_table(flaky, 100)
    out += ['', '</details>', '']
  return f"{'\n'.join(out)}\n"


def known_differences_lines(items):
  """Formats items as known-differences.txt lines."""
  return [
      ' | '.join(
          _pattern(part) for part in (item['test'], item['subtest'])
          if part is not None) for item in items
  ]


def _pattern(text):
  # Escape the glob characters, and replace what a line of the file cannot
  # hold (line breaks, the field separator) with wildcards.
  text = re.sub(r'([*?\[])', r'[\1]', text.strip())
  return re.sub(r'\s*\n\s*', '*', text).replace(' | ', ' ? ')
