# Web Platform Tests

This directory runs [web-platform-tests](https://web-platform-tests.org/) (WPT)
against Electron on Linux and compares the results with Chromium's, to find
tests that Chromium passes and Electron fails.

Chromium is never run here. The results to compare with come from one of two
places, chosen by the WPT revision:

| Revision | Tests | Baseline |
|---|---|---|
| `pinned` | The copy of WPT in Chromium at the version in `DEPS` (`third_party/blink/web_tests/external/wpt`) | Chromium's own expectations for those tests at that version: `TestExpectations` and the `-expected.txt` files |
| `latest` | The newest upstream WPT commit that has a complete Chrome Canary run on [wpt.fyi](https://wpt.fyi) | The results of that Chrome Canary run |

`pinned` answers "does Electron pass what the Chromium it is built from
passes?" and is what pull requests use. `latest` answers "does this branch pass
what Chrome Canary passes today?", which also shows web platform fixes a release
branch is missing.

## Running locally

You need a Linux build of Electron with its ChromeDriver (`e build` followed by
`e build -t electron:electron_chromedriver_zip`, or `dist.zip` and
`chromedriver.zip` from CI unpacked into one directory), plus Python 3, Git and
Xvfb. `pinned` also needs the Chromium checkout Electron is built in (the
directory above this repository, or `--chromium-src`) and `vpython3` from
depot_tools on `PATH`.

```sh
# Copy the tests out of the Chromium checkout and build the baseline (about
# 700 MB for pinned).
python3 script/wpt/wpt.py prepare --work /tmp/wpt --revision pinned --keep-venv

# The wpt servers answer for web-platform.test and its subdomains.
python3 script/wpt/wpt.py hosts --work /tmp/wpt | sudo tee -a /etc/hosts

# Run some tests, or leave the paths out to run everything.
python3 script/wpt/wpt.py run --work /tmp/wpt --electron-dir ../out/Testing \
  --out /tmp/wpt-reports dom/events css/css-backgrounds

# Compare with the baseline.
python3 script/wpt/wpt.py summarize --work /tmp/wpt --reports /tmp/wpt-reports
```

Testing builds have DCHECKs on and need `--strip-feature-params` on `run`
until [#55990](https://github.com/electron/electron/issues/55990) is fixed.

Tests that fake a device or a service (WebXR, Shape Detection, Web NFC,
Battery Status and others) do it with MojoJS bindings that the wpt server
serves at `/gen/`. Without `--mojojs`, `run` uses the bindings published with
Chrome for Testing, which only match release builds of Electron: official
builds scramble mojom message IDs, so in a testing build these tests fail with
`Received unknown message`. To use the bindings of the build instead, as
Chromium's own runs do, build them and pass the build's `gen` directory:

```sh
e build -t electron:electron_wpt_mojojs
python3 script/wpt/wpt.py run --work /tmp/wpt --electron-dir ../out/Testing \
  --mojojs ../out/Testing/gen --out /tmp/wpt-reports webxr
```

## What runs where

- `wpt.py` is the entry point: `prepare`, `run`, `summarize` and `hosts`,
  and `mojojs`, which packages the MojoJS bindings of a build as `mojojs.zip`.
- `baseline_chromium.py` builds the `pinned` baseline with Chromium's own
  `blinkpy`, from the files `prepare` copies out of the Chromium checkout.
- `baseline_wptfyi.py` builds the `latest` baseline from the wpt.fyi API.
- `diff.py` compares reports with a baseline and writes the summary.
- `product/` is a wptrunner product named `electron`. It reuses wptrunner's
  Chrome support and adds what Electron needs: the shell app, a private X
  server per browser instead of headless mode, and a reftest executor that
  does not resize the window.
- `wpt_cli.py` starts the `wpt` command with that product available.
- `shell/` is the Electron app the tests run in. It opens one window and
  mirrors Chrome's defaults for permissions and popups.
- `expectations/` holds the two lists of known differences described below.

For `pinned`, the tests are run with the copy of wptrunner in Chromium
(`third_party/wpt_tools`) and the browser switches Chromium's
`run_wpt_tests.py` uses for `headless_shell`, so that results differ because
of Electron and not because of configuration. Chromium's Python tools are run
the way Chromium runs them, with depot_tools' `vpython3` and the `vpython.toml`
in the Chromium checkout, which provides the Python version and the packages
they need. `latest` runs upstream WPT, which has no such file, in a virtualenv
with WPT's own requirements. Chromium's expectations are also made with a
fixed set of fonts, `third_party/test_fonts`, and no others, so `pinned` gives
Electron the same ones through `FONTCONFIG_SYSROOT`, as `run_wpt_tests.py`
does. What could not be matched for `pinned`: Electron runs in a window under
Xvfb instead of headless, and a few of the switches are specific to
`headless_shell` and have no effect on Electron. `--headless` also turns
Chromium's `PaintHolding` feature off, so the `electron` product passes
`--disable-features=PaintHolding` for both revisions.

Chromium does not run every WPT that way. The tests listed in
`third_party/blink/web_tests/TestLists/content_shell.filter` run in
`content_shell`, which sends input through its own test APIs instead of
ChromeDriver, and the ones in `chrome.filter` run in Chrome. Chromium's
expectations for those tests do not describe a ChromeDriver run, so `pinned`
leaves them out, as Chromium's `headless_shell` suite does.

## Known differences

A test that passes in the baseline and fails on Electron fails the run, unless
it is listed in one of these files:

- `expectations/exclude.txt`: tests that are not run at all.
- `expectations/known-differences.txt`: failures that are expected. Each line
  is a pattern for the test URL, optionally followed by patterns for the
  subtest name and the failure message.

The summary step uploads a `wpt-diff` artifact. Its `new-failures.txt` has one
line per new failure in the format of `known-differences.txt`, ready to be
copied over once a failure is understood.

Tests with new failures are run a second time. Those that pass then are
reported as flaky and do not fail the run. Tests that timed out are not run
again and count as failures.

## CI

- Adding the `run-wpt` label to a pull request runs the `pinned` comparison
  for it. For branches in this repository the `WPT` workflow starts right
  away; for forks it runs as part of the `Build` workflow from the next push.
- The `WPT` workflow (`.github/workflows/wpt.yml`) runs every Monday on `main`
  with `latest`, and starts itself on every supported release branch. It can
  also be started by hand with either revision.

Both use `.github/workflows/pipeline-segment-wpt.yml`, which splits the tests
over 16 shards and writes the comparison to the run summary. For `pinned`, its
`Prepare` job takes Chromium's files from the src cache the checkout job of
the same run saved. Each shard also uploads wptrunner's HTML report of its
results as `wpt-html-<shard>.html`, which opens in the browser from the run's
page. The build job generates the MojoJS bindings
(`//electron:electron_wpt_mojojs`) and adds them to its artifacts as
`mojojs.zip`, which the shards pass to `run --mojojs`. Only the `testharness`, `reftest` and `crashtest` test types are run for
now.
