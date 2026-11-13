import { app } from 'electron/main';

import * as fs from 'node:fs';
import * as path from 'node:path';

// A minimal spec worker app for spec/vitest-pool.spec.ts: the Electron pool
// starts it, and it hands itself to vitest's worker runtime once ready, as
// spec/index.js does.

// The pool makes each worker lead its own process group, out of reach of the
// outer run's cleanup, so record it for spec/vitest-pool.spec.ts to kill if
// this run times out. A worker that quits removes its record, so that the spec
// never signals its pid after the system may have reused it.
const pidFile = path.join(process.env.ELECTRON_SPEC_FIXTURE_TMP_DIR, `worker-${process.pid}.pid`);
fs.writeFileSync(pidFile, '');
process.on('exit', () => fs.rmSync(pidFile, { force: true }));

app.whenReady().then(() => import('../../../vitest/worker.ts'));
