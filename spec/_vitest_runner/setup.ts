import * as chai from 'chai';
import { afterAll, beforeEach } from 'vitest';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { runCleanupFunctions } from '../lib/defer-helpers';
import { assertNoWindowsLeaked } from '../lib/window-helpers';

import chaiAsPromised = require('chai-as-promised');
import dirtyChai = require('dirty-chai');

chai.use(chaiAsPromised);
chai.use(dirtyChai);

// Show full object diff.
// https://github.com/chaijs/chai/issues/469
chai.config.truncateThreshold = 0;

// Skip any tests listed in disabled-tests.json.
const disabledTests = new Set(JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'disabled-tests.json'), 'utf8')));
beforeEach((ctx) => {
  // Fallback drain for defer()ed cleanups. Most tests use afterEach(closeAllWindows)
  // which already runs these first; this catches tests that defer() without it.
  // Note: onTestFinished runs *after* afterEach, so callbacks reaching here must
  // not assume windows still exist.
  ctx.onTestFinished(runCleanupFunctions);

  const parts: string[] = [ctx.task.name];
  let suite = ctx.task.suite;
  while (suite) {
    if (suite.name) parts.unshift(suite.name);
    suite = suite.suite;
  }
  if (disabledTests.has(parts.join(' '))) {
    ctx.skip();
  }
});

// Runs once per file, after all test-file-level afterAll hooks (setupFiles
// hooks are outermost). Using afterAll rather than afterEach so suites that
// intentionally share a window across tests (useRemoteContext, etc.) are not
// flagged on every test.
afterAll(assertNoWindowsLeaked);
