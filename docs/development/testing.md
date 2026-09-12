# Testing

We aim to keep the code coverage of Electron high. We ask that all pull
request not only pass all existing tests, but ideally also add new tests
to cover changed code and new scenarios. Ensuring that we capture as
many code paths and use cases of Electron as possible ensures that we
all ship apps with fewer bugs.

This repository comes with linting rules for both JavaScript and C++ –
as well as unit and integration tests. To learn more about Electron's
coding style, please see the [coding-style](coding-style.md) document.

## Linting

To ensure that your changes are in compliance with the Electron coding
style, run `npm run lint`, which will run a variety of linting checks
against your changes depending on which areas of the code they touch.

Many of these checks are included as precommit hooks, so it's likely
your error would be caught at commit time.

## Unit Tests

If you are not using [build-tools](https://github.com/electron/build-tools),
ensure that the name you have configured for your
local build of Electron is one of `Testing`, `Release`, `Default`, or
you have set `process.env.ELECTRON_OUT_DIR`. Without these set, Electron will fail
to perform some pre-testing steps.

To run all unit tests, run `npm run test`. The unit tests are an Electron
app (surprise!) that can be found in the `spec` folder. Note that it has
its own `package.json` and that its dependencies are therefore not defined
in the top-level `package.json`.

To run only specific tests matching a pattern, run `npm run test --
-g=PATTERN`, replacing the `PATTERN` with a regex that matches the tests
you would like to run. As an example: If you want to run only IPC tests, you
would run `npm run test -- -g ipc`.

## Printer tray and media tests

Run the captured-job regressions with:

```sh
node script/spec-runner.js --runners=main --files=spec/api-web-contents-spec.ts --grep='webContents.print.. settings'
```

On macOS and Linux the runner can provision an `ippeveprinter` and a CUPS queue
when `ippeveprinter`, `lpadmin`, Python 3 and permission to administer CUPS are
available. It uses [a fixed printer fixture](../../spec/fixtures/printing/ipp-printer.conf)
and [a capture command](../../spec/fixtures/printing/capture-job.py). Automatic
provisioning is best-effort. The capture regressions skip when
`ELECTRON_TEST_PRINT_CAPTURE` is unset; a configured capture fixture must work.

For CI runners without permission to administer CUPS, provision the fixture
before starting the test runner and export the variables below. Require the
fixture only in shards whose final file selection includes
`spec/api-web-contents-spec.ts`, after applying any display-server filter.

The HTML and PDF tests inspect the received PDF and IPP job attributes for a
baseline, a non-default tray, a non-default media type, both selections, and a
subsequent job with neither selection. They also check paper dimensions and
unchanged reported printer defaults. Callback success alone does not pass.

To use an existing **dedicated virtual queue**, set `ELECTRON_TEST_PRINTER_NAME`
and `ELECTRON_TEST_PRINT_CAPTURE`. The latter is a JSON object, for example:

```json
{
  "deviceName": "electron-print-test",
  "jobsDirectory": "/tmp/electron-print-capture",
  "inputTray": { "id": "Manual", "ipp": "manual" },
  "mediaType": { "id": "Labels", "ipp": "labels" }
}
```

Use IDs returned by `getPrinterCapabilitiesAsync()` for the selected queue.
The fixture's Linux PPD choices are `Manual` and `Labels`; on macOS the IPP IDs
are `manual` and `labels`. Both must differ from the queue's effective defaults.
Configured fixtures must be available and expose these choices; the tests fail
instead of skipping when a configured fixture is missing or incompatible.

For Windows, register the fixture's IPP endpoint using `Add-Printer -IppURL` on a
Windows version supporting that command, and obtain that driver's decimal tray
and media IDs. For example, Microsoft's IPP Class Driver may report `258` for
manual feed and `259` for labels; verify these IDs on the test machine. Serve
the capture directory with
`python3 spec/fixtures/printing/capture-server.py DIRECTORY --host TEST_HOST`.
Replace `jobsDirectory` in the configuration with
`"jobsUrl": "http://TEST_HOST:8633/jobs"`. This also supports other remote runners.
The default Windows Print-to-PDF smoke fixture has no tray/media capture; these
regressions require the IPP capture fixture.

### Linux printer credentials

In an isolated Linux test container, configure a second virtual IPP printer with
`ippeveprinter -A --pam-service TEST_PAM_SERVICE -a spec/fixtures/printing/ipp-printer.conf`,
the capture command and spool directory. Use a disposable PAM service/account,
and register its CUPS queue with `auth-info-required=username,password`.
The test credential below is `electron-print-fixture` / `fixture-only`.

Install `dbus-run-session`, `python3-dbus` and `python3-gi`. Add
`"authenticationReport": "/tmp/printing-auth.json"` to the capture configuration,
then run:

```sh
python3 spec/fixtures/printing/secret-service.py \
  --printer electron-print-test --report /tmp/printing-auth.json -- \
  node script/spec-runner.js --runners=main --files=spec/api-web-contents-spec.ts \
  --grep='webContents.print.. settings'
```

This starts a private session bus with a disposable Secret Service. The tests
require a credential lookup and successful captured jobs with non-default
tray/media settings. The report records counts, and the capture command records
only selected print attributes; neither records credentials.

## Node.js Smoke Tests

If you've made changes that might affect the way Node.js is embedded into Electron,
we have a test runner that runs all of the tests from Node.js, using Electron's custom fork
of Node.js.

To run all of the Node.js tests:

```bash
$ node script/node-spec-runner.js
```

To run a single Node.js test:

```bash
$ node script/node-spec-runner.js parallel/test-crypto-keygen
```

where the argument passed to the runner is the path to the test in
the Node.js source tree.

### Testing on Windows 10 devices

#### Extra steps to run the unit test:

1. Visual Studio 2019 must be installed.
2. Node headers have to be compiled for your configuration.

   ```powershell
   ninja -C out\Testing electron:node_headers
   ```

3. The electron.lib has to be copied as node.lib.

   ```powershell
   cd out\Testing
   mkdir gen\node_headers\Release
   copy electron.lib gen\node_headers\Release\node.lib
   ```

#### Missing fonts

[Some Windows 10 devices](https://learn.microsoft.com/en-us/typography/fonts/windows_10_font_list) do not ship with the Meiryo font installed, which may cause a font fallback test to fail. To install Meiryo:

1. Push the Windows key and search for _Manage optional features_.
2. Click _Add a feature_.
3. Select _Japanese Supplemental Fonts_ and click _Install_.

#### Pixel measurements

Some tests which rely on precise pixel measurements may not work correctly on
devices with Hi-DPI screen settings due to floating point precision errors.
To run these tests correctly, make sure the device is set to 100% scaling.

To configure display scaling:

1. Push the Windows key and search for _Display settings_.
2. Under _Scale and layout_, make sure that the device is set to 100%.

## Multi-Monitor Tests

Some Electron APIs require testing across multiple displays, such as screen detection, window positioning, and display-related events. For contributors working on these features, the `virtualDisplay` native addon enables you to create and position virtual displays programmatically, making it possible to test multi-monitor scenarios without any physical hardware.

For detailed information on using virtual displays in your tests, see [Multi-Monitor Testing](multi-monitor-testing.md).

**Platform support:** macOS only
