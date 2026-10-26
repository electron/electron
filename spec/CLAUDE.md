# Writing specs

## Child processes and stdio

- Never call `expect()` inside a stream or event listener (`'data'`, `'message'`,
  `'exit'`). A failing assertion there is an uncaught exception, not a test
  failure: the spec runner exits with no summary and no JUnit file, and the whole
  shard goes red with nothing to point at. Collect what you need in the listener,
  and assert in the test body after `await`ing whatever you are waiting for.
- Wait for the output you expect, not for `'exit'`. A chunk the child wrote
  just before exiting can still be in the pipe when `'exit'` fires and only be
  delivered afterwards. Accumulate output until the expected pattern appears
  (see `outputUntil` in `api-utility-process.spec.ts`), then assert on it.
- Don't assert on the first chunk from a pipe. stdout and stderr race, and a
  stray warning on stderr (a `net/dns` config warning, a GPU message) can land
  before the line you want. Match a pattern against the accumulated output.
- Register every forked child with `deferKillUtilityProcess` (or `defer` a kill
  for `child_process` children) right after creating it. If the expected output
  never arrives the test fails on its timeout and the code path that would
  have killed the child is never reached; the global `afterEach` then reaps it.
  A leaked child holds its ports (`--inspect=<port>`) and breaks the in-job
  retry of the same test.
- Only kill a child that is still running (`child.pid` is set). Killing and then
  `await once(child, 'exit')` on a child that already exited hangs until the
  test times out.

## Several spec files run at once

- The runner starts several Electron processes and gives each one spec file at
  a time, so other windows, from other processes, exist while a test runs.
  A test that needs OS-level focus, reads or writes the system clipboard,
  captures the screen, registers global shortcuts, moves the real mouse, or
  talks to the mocked D-Bus services must be inside a suite declared with
  `describe(title, { tags: ['serial'] }, fn)` (or `it(title, { tags:
  ['serial'] }, fn)`); those run afterwards, one file at a time, with nothing
  else on screen.
- Each process has its own `userData` directory; do not assume the default
  path, ask `app.getPath('userData')`.
