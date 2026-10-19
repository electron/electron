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
//    Each cleanup gets the hook timeout, as an afterEach hook would.
//  - A failing afterEach hook or defer()-ed cleanup does not stop the rest.
//    Once one throws, vitest skips the test's remaining afterEach hooks,
//    the enclosing suites' included,
//    so one failed cleanup would leave a protocol handler or a window behind
//    and break the tests after it.
//    Each error is kept instead and thrown once every hook has run,
//    so the test still fails with all of them.
//  - spec/disabled-tests.json switches a test off by its full title without
//    touching the spec file (used to unblock a roll while a fix is pending).

import { TestRunner } from 'vitest';

import * as fs from 'node:fs';
import * as path from 'node:path';
import { setImmediate } from 'node:timers/promises';

import { runCleanupFunctions } from '../lib/spec-helpers.ts';

import type { RunnerTask, RunnerTestCase, RunnerTestFile, RunnerTestSuite, TestContext, TestTryOptions } from 'vitest';

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

// What the afterEach hooks and defer()-ed cleanup of each running test's
// current try have thrown so far.
const cleanupErrors = new WeakMap<object, unknown[]>();

function collectingErrors(hook: (context: TestContext, suite: Readonly<RunnerTestSuite>) => unknown) {
  return async function (this: unknown, context: TestContext, suite: Readonly<RunnerTestSuite>) {
    try {
      await hook.call(this, context, suite);
    } catch (error) {
      const errors = cleanupErrors.get(context.task);
      // Nothing is collecting for this test, so let vitest report it now.
      if (!errors) throw error;
      errors.push(error);
    }
  };
}

export default class ElectronSpecRunner extends TestRunner {
  async onBeforeRunSuite(suite: RunnerTestSuite) {
    if (!prepared.has(suite)) {
      prepared.add(suite);
      const hooks = TestRunner.getSuiteHooks(suite);
      for (const list of [hooks.beforeAll, hooks.beforeEach, hooks.afterEach, hooks.afterAll]) {
        list.splice(0, list.length, ...list.map(fromFreshMacrotask));
      }
      // Read when the hook runs, so that vi.setConfig() applies to it.
      hooks.afterEach.unshift(() => runCleanupFunctions(this.config.hookTimeout));
      hooks.afterEach.splice(0, hooks.afterEach.length, ...hooks.afterEach.map(collectingErrors));
    }
    return super.onBeforeRunSuite(suite);
  }

  async onBeforeRunTask(test: RunnerTestCase) {
    if (disabledTests.has(fullTitle(test))) test.mode = 'skip';
    return super.onBeforeRunTask(test);
  }

  async onBeforeTryTask(test: RunnerTestCase, options: TestTryOptions) {
    await setImmediate();
    const errors: unknown[] = [];
    cleanupErrors.set(test, errors);
    // vitest runs onTestFinished callbacks after all of the afterEach hooks,
    // newest first, so this one also runs after any the test adds.
    test.context.onTestFinished(() => {
      cleanupErrors.delete(test);
      if (errors.length === 1) throw errors[0];
      if (errors.length > 1) {
        // vitest reports each error in an AggregateError, but only one level
        // deep, so take apart the one runCleanupFunctions throws.
        const all = errors.flatMap((error) => (error instanceof AggregateError ? error.errors : [error]));
        throw new AggregateError(all, 'Several afterEach hooks or defer()-ed cleanup functions failed');
      }
    });
    return super.onBeforeTryTask(test, options);
  }
}
