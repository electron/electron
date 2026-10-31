import { app } from 'electron/main';

import { expect, it } from 'vitest';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { waitUntil } from '../../../lib/spec-helpers.ts';

const tmpDir = process.env.ELECTRON_SPEC_FIXTURE_TMP_DIR!;
const timeout = Number(process.env.ELECTRON_SPEC_TIMEOUT) || 30_000;

it('passes', async () => {
  if (process.env.ELECTRON_SPEC_FIXTURE_EXIT_WHILE_QUITTING) {
    // Exit with an error code once quitting, as a crash during shutdown would.
    app.once('will-quit', () => app.exit(1));
  }
  if (process.env.ELECTRON_SPEC_FIXTURE_EXIT_AFTER_SIGTERM) {
    // Keep quitting until the pool sends SIGTERM, then exit with an error code
    // instead of quitting: a failure of the worker's own, after that signal.
    app.once('will-quit', (event) => event.preventDefault());
    process.once('SIGTERM', () => app.exit(1));
  }
  if (process.env.ELECTRON_SPEC_FIXTURE_WAIT) {
    // Finish only once wait.fixture.ts runs, so that it is still running when
    // this worker quits.
    await waitUntil(() => fs.existsSync(path.join(tmpDir, 'waiting')), { timeout });
  }
  expect(true).toBe(true);
});
