import { it } from 'vitest';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { waitUntil } from '../../../lib/spec-helpers.ts';

const tmpDir = process.env.ELECTRON_SPEC_FIXTURE_TMP_DIR!;
const timeout = Number(process.env.ELECTRON_SPEC_TIMEOUT) || 30_000;

// Runs beside quit.fixture.ts and keeps the run going until this vitest has
// handled the exit of that file's worker (see vitest.config.ts), so that the
// worker quits while a spec file is still running.
it('waits for the other worker to exit', async () => {
  fs.writeFileSync(path.join(tmpDir, 'waiting'), '');
  await waitUntil(() => fs.existsSync(path.join(tmpDir, 'worker-exited')), { timeout });
});
