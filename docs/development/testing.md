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

The suite is driven by [vitest](https://vitest.dev): the `vitest` CLI runs
under Node.js and starts several instances of the `spec` app, each of which
runs one spec file at a time in Electron's main process. Suites that need the
machine to themselves (window focus, the clipboard, the screen, global
shortcuts) are declared with `describe(title, { tags: ['serial'] }, fn)` and
run one at a time after everything else has finished. Set
`ELECTRON_SPEC_WORKERS` to change how many Electron processes run at once.

To run only some spec files, pass them with `--files`:
`npm run test -- --files spec/api-app.spec.ts`. To run only tests whose
name matches a pattern, run `npm run test -- -g=PATTERN`, replacing
`PATTERN` with a regex. As an example: If you want to run only IPC tests, you
would run `npm run test -- -g ipc`.

The spec files use mocha's interface (`describe`, `it`, `before`, `after`,
`this.timeout()`), provided on top of vitest by `spec/vitest/mocha-compat.ts`,
and [chai](https://www.chaijs.com) assertions.

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
