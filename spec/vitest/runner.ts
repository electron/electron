// vitest `runner`: the stock test runner plus the behaviour the Electron specs
// rely on that a test framework does not otherwise provide.
//
//  - Runnables start from a fresh macrotask. vitest chains a file's hooks and
//    tests as promise continuations, so without this nothing else on the main
//    process event loop (Chromium tasks, IPC, window teardown) runs between
//    one test's end and the next one's start, and a spec that destroys a
//    window in afterEach and creates one in the next beforeEach trips over
//    the half torn down one. mocha did the same (Runner.immediately).
//  - `defer()`-ed cleanup (spec/lib/spec-helpers.ts) runs straight after each
//    test, before the enclosing suites' own afterEach hooks, so that a hook
//    like `afterEach(closeAllWindows)` finds the test's windows already gone.
//  - spec/disabled-tests.json switches a test off by its full title without
//    touching the spec file (used to unblock a roll while a fix is pending).

import { TestRunner } from 'vitest';

import * as fs from 'node:fs';
import * as path from 'node:path';
import { setImmediate } from 'node:timers/promises';

import { runCleanupFunctions } from '../lib/spec-helpers.ts';

import type { RunnerTask, RunnerTestCase, RunnerTestFile, RunnerTestSuite, TestTryOptions } from 'vitest';

const disabledTests = new Set<string>(
  JSON.parse(fs.readFileSync(path.join(import.meta.dirname, '..', 'disabled-tests.json'), 'utf8'))
);

function fullTitle(task: RunnerTask): string {
  const parts: string[] = [];
  // The outermost suite is the file itself, whose name is its path.
  for (let t: RunnerTask | undefined = task; t && !('filepath' in t); t = t.suite) parts.unshift(t.name);
  return parts.join(' ');
}

const prepared = new WeakSet<RunnerTestSuite | RunnerTestFile>();

const fromFreshMacrotask = <T extends (...args: any[]) => unknown>(fn: T) =>
  async function (this: unknown, ...args: Parameters<T>) {
    await setImmediate();
    return fn.apply(this, args);
  } as T;

export default class ElectronSpecRunner extends TestRunner {
  async onBeforeRunSuite(suite: RunnerTestSuite) {
    if (!prepared.has(suite)) {
      prepared.add(suite);
      const hooks = TestRunner.getSuiteHooks(suite);
      for (const list of [hooks.beforeAll, hooks.beforeEach, hooks.afterEach, hooks.afterAll]) {
        list.splice(0, list.length, ...list.map(fromFreshMacrotask));
      }
      hooks.afterEach.unshift(runCleanupFunctions);
    }
    return super.onBeforeRunSuite(suite);
  }

  async onBeforeRunTask(test: RunnerTestCase) {
    if (disabledTests.has(fullTitle(test))) test.mode = 'skip';
    return super.onBeforeRunTask(test);
  }

  async onBeforeTryTask(test: RunnerTestCase, options: TestTryOptions) {
    await setImmediate();
    return super.onBeforeTryTask(test, options);
  }
}
