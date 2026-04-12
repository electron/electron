import * as chai from 'chai';
import { beforeEach } from 'vitest';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { runCleanupFunctions } from '../lib/defer-helpers';

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
  // Run defer()-ed cleanup functions immediately after the test body, before any
  // afterEach hooks registered by the test file. onTestFinished fires before
  // afterEach, so defer() cleanups precede test-file afterEach hooks.
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
