// The spec files were written for mocha's BDD interface: `describe`/`it`/
// `before`/`after` globals, `function () { this.timeout(n); this.skip(); }`
// contexts and `done` callbacks. This module provides those globals on top of
// vitest so the files run unmodified. The vitest-native three argument form,
// `it(name, { timeout, retry, tags }, fn)`, is accepted as well, and test
// functions also receive vitest's TestContext as their first argument when they
// do not take a `done` callback.
//
// Timeouts are implemented here rather than by vitest (the config sets
// vitest's own to 0) because mocha's are resettable from inside a running test
// or hook (`this.timeout(60000)` half way through), which vitest's are not.

import * as vitest from 'vitest';

import type { SuiteOptions, TestContext, TestOptions } from 'vitest';

const DEFAULT_TIMEOUT = Number(process.env.MOCHA_TIMEOUT) || 30_000;
const DEFAULT_RETRIES = process.env.CI ? 3 : 0;

type Done = (err?: unknown) => void;
type MochaFn = (this: MochaContext, doneOrCtx?: any) => unknown;
type SuiteFn = (this: SuiteThis) => unknown;

interface SuiteThis {
  timeout(ms?: number): number | SuiteThis;
  retries(n?: number): number | SuiteThis;
  slow(): SuiteThis;
  bail(): SuiteThis;
}

interface SuiteContext {
  readonly parent: SuiteContext | null;
  readonly title: string;
  timeoutMs: number | undefined;
  retryCount: number | undefined;
}

const rootSuite: SuiteContext = { parent: null, title: '', timeoutMs: undefined, retryCount: undefined };
let currentSuite: SuiteContext = rootSuite;

function inheritedTimeout(suite: SuiteContext | null): number {
  for (let s = suite; s; s = s.parent) {
    if (s.timeoutMs !== undefined) return s.timeoutMs;
  }
  return DEFAULT_TIMEOUT;
}

function inheritedRetries(suite: SuiteContext | null): number {
  for (let s = suite; s; s = s.parent) {
    if (s.retryCount !== undefined) return s.retryCount;
  }
  return DEFAULT_RETRIES;
}

/**
 * What mocha binds as `this` inside tests and hooks.
 */
export class MochaContext {
  #timeoutMs: number;
  #onTimeoutChange: (ms: number) => void;
  #vitestCtx: TestContext | undefined;
  #title: string;

  constructor(timeoutMs: number, onTimeoutChange: (ms: number) => void, title: string, vitestCtx?: TestContext) {
    this.#timeoutMs = timeoutMs;
    this.#onTimeoutChange = onTimeoutChange;
    this.#vitestCtx = vitestCtx;
    this.#title = title;
  }

  timeout(ms?: number | string): any {
    if (ms === undefined) return this.#timeoutMs;
    this.#timeoutMs = typeof ms === 'string' ? parseInt(ms, 10) : ms;
    this.#onTimeoutChange(this.#timeoutMs);
    return this;
  }

  slow(): this {
    return this;
  }

  retries(n?: number): any {
    if (n === undefined) return this.#vitestCtx?.task.retry ?? 0;
    // vitest fixes the retry count when the test is registered; use
    // `this.retries(n)` in the enclosing describe or `it(name, { retry: n }, fn)`.
    throw new Error('this.retries() cannot be changed from inside a running test under vitest');
  }

  skip(): never {
    if (this.#vitestCtx) {
      this.#vitestCtx.skip();
    }
    // beforeAll/afterAll have no per-test context; mimic mocha's pending signal.
    const err: any = new Error('sync skip; aborting execution');
    err.pending = true;
    throw err;
  }

  get test() {
    const ctx = this.#vitestCtx;
    const title = this.#title;
    return {
      title,
      fullTitle: () => (ctx ? taskFullTitle(ctx.task) : title),
      isFailed: () => ctx?.task.result?.state === 'fail',
      isPassed: () => ctx?.task.result?.state === 'pass',
      get state() {
        const state = ctx?.task.result?.state;
        return state === 'fail' ? 'failed' : state === 'pass' ? 'passed' : undefined;
      }
    };
  }

  get currentTest() {
    return this.test;
  }
}

function taskFullTitle(task: { name: string; suite?: any; file?: any }): string {
  const parts = [task.name];
  for (let s = task.suite; s && s !== task.file && s.name; s = s.suite) parts.unshift(s.name);
  return parts.join(' ');
}

class TimeoutError extends Error {
  constructor(ms: number, what: string) {
    super(
      `Timeout of ${ms}ms exceeded. For async tests and hooks, ensure "done()" is called; if returning a Promise, ensure it resolves. (${what})`
    );
    this.name = 'TimeoutError';
    (this as any).code = 'ERR_MOCHA_TIMEOUT';
  }
}

/**
 * Runs `fn` the way mocha would: `this` bound to a MochaContext, an optional
 * `done` callback, and a resettable timeout.
 */
// Marks every test under a vitest suite as skipped; mocha does this when a
// 'before all' hook calls this.skip().
function skipSuiteTasks(suiteTask: any) {
  for (const task of suiteTask?.tasks ?? []) {
    if (task.type === 'suite') {
      // Keep nested suites runnable so the runner still visits and reports
      // each test, but drop their hooks: mocha runs none for a skipped suite.
      const hooks = vitest.TestRunner.getSuiteHooks(task);
      if (hooks) {
        for (const list of Object.values(hooks) as unknown[][]) list.length = 0;
      }
      skipSuiteTasks(task);
    } else {
      task.mode = 'skip';
      // Collection has already been reported, so give the test a result the
      // reporters recognise instead of relying on the mode alone.
      task.result = { state: 'skip' };
    }
  }
}

async function runMochaStyle(
  fn: MochaFn,
  suite: SuiteContext,
  what: string,
  title: string,
  vitestCtx?: TestContext,
  explicitTimeout?: number,
  vitestSuite?: unknown
): Promise<void> {
  return new Promise<void>((resolve, reject) => {
    let timer: NodeJS.Timeout | undefined;
    let settled = false;
    const finish = (err?: unknown) => {
      if (settled) return;
      settled = true;
      if (timer) clearTimeout(timer);
      if (err) {
        reject(err instanceof Error ? err : new Error(`the ${what} was rejected with a non-error: ${String(err)}`));
      } else {
        resolve();
      }
    };
    const armTimer = (ms: number) => {
      if (timer) clearTimeout(timer);
      timer = undefined;
      // mocha treats 0 (and anything that overflows a 32-bit timer) as "no timeout".
      if (ms > 0 && ms < 2 ** 31) {
        timer = setTimeout(() => finish(new TimeoutError(ms, what)), ms);
      }
    };

    const ctx = new MochaContext(explicitTimeout ?? inheritedTimeout(suite), armTimer, title, vitestCtx);
    armTimer(ctx.timeout());

    try {
      if (vitestStyle.has(fn)) {
        Promise.resolve(fn.call(ctx, vitestCtx)).then(() => finish(), finish);
      } else if (fn.length > 0) {
        // mocha `done` style. Returning a promise as well is an error in mocha;
        // here the callback simply wins.
        const done: Done = (err) => finish(err ?? undefined);
        fn.call(ctx, done);
      } else {
        // Like mocha, pass nothing: helpers such as closeAllWindows(assert = false)
        // are used directly as hooks and would misread an argument.
        Promise.resolve(fn.call(ctx)).then(() => finish(), finish);
      }
    } catch (err) {
      finish(err);
    }
  }).catch((err) => {
    if (err?.pending) {
      // this.skip(): from a test or an *Each hook it skips that test, from a
      // 'before all' hook it skips the whole suite, as in mocha.
      if (vitestCtx) vitestCtx.skip();
      else skipSuiteTasks(vitestSuite);
      return;
    }
    throw err;
  });
}

// Functions declared through the vitest-style `(ctx) => {}` signature are
// indistinguishable by arity from `(done) => {}`, so tests that want vitest's
// TestContext argument opt in with `it(name, { context: true }, fn)`.
const vitestStyle = new WeakSet<Function>();

type ItOptions = TestOptions & { context?: boolean };

function splitArgs<O>(optionsOrFn?: O | MochaFn, maybeFn?: MochaFn): [O | undefined, MochaFn | undefined] {
  if (typeof optionsOrFn === 'function') return [undefined, optionsOrFn as MochaFn];
  return [optionsOrFn as O | undefined, maybeFn];
}

function makeIt(mode: 'run' | 'only' | 'skip') {
  return function it(title: string, optionsOrFn?: ItOptions | MochaFn, maybeFn?: MochaFn) {
    const [options, fn] = splitArgs<ItOptions>(optionsOrFn, maybeFn);
    const suite = currentSuite;
    const { context, timeout, ...vitestOptions } = options ?? {};
    if (fn && context) vitestStyle.add(fn);
    const testOptions: TestOptions = { retry: inheritedRetries(suite), ...vitestOptions };
    // mocha's `it(...).timeout(ms)` chain; read when the test runs.
    const overrides = { timeout };
    const chain = {
      timeout(ms: number) {
        overrides.timeout = ms;
        return chain;
      },
      slow: () => chain,
      retries() {
        throw new Error('it(...).retries(n) is not supported under vitest; use it(name, { retry: n }, fn)');
      }
    };

    if (!fn) {
      vitest.it.todo(title);
      return chain;
    }
    if (mode === 'skip') {
      vitest.it.skip(title, testOptions, () => {});
      return chain;
    }
    // See makeHook for why this reads the context from `arguments`.
    const body = function () {
      return runMochaStyle(fn, suite, 'test', title, arguments[0] as TestContext, overrides.timeout);
    };
    if (mode === 'only') vitest.it.only(title, testOptions, body);
    else vitest.it(title, testOptions, body);
    return chain;
  };
}

function makeDescribe(mode: 'run' | 'only' | 'skip') {
  return function describe(title: string, optionsOrFn?: SuiteOptions | SuiteFn, maybeFn?: SuiteFn) {
    const [options, fn] = typeof optionsOrFn === 'function' ? [undefined, optionsOrFn] : [optionsOrFn, maybeFn];
    const suite: SuiteContext = { parent: currentSuite, title, timeoutMs: undefined, retryCount: undefined };
    const { timeout, retry, ...suiteOptions } = options ?? {};
    if (timeout !== undefined) suite.timeoutMs = timeout;
    if (retry !== undefined && typeof retry === 'number') suite.retryCount = retry;

    const factory = async () => {
      const previous = currentSuite;
      currentSuite = suite;
      try {
        const suiteThis: SuiteThis = {
          timeout(ms?: number) {
            if (ms === undefined) return inheritedTimeout(suite);
            suite.timeoutMs = ms;
            return this;
          },
          retries(n?: number) {
            if (n === undefined) return inheritedRetries(suite);
            suite.retryCount = n;
            return this;
          },
          slow() {
            return this;
          },
          // mocha's suite-level bail has no per-suite equivalent in vitest.
          bail() {
            return this;
          }
        };
        if (fn) await fn.call(suiteThis);
      } finally {
        currentSuite = previous;
      }
    };

    if (mode === 'skip') return vitest.describe.skip(title, suiteOptions, factory);
    if (mode === 'only') return vitest.describe.only(title, suiteOptions, factory);
    return vitest.describe(title, suiteOptions, factory);
  };
}

type HookRegistrar = typeof vitest.beforeAll | typeof vitest.beforeEach;

function makeHook(register: HookRegistrar, kind: string, perTest: boolean) {
  return function hook(titleOrFn: string | MochaFn, maybeFn?: MochaFn | number, maybeTimeout?: number) {
    const fn = (typeof titleOrFn === 'function' ? titleOrFn : maybeFn) as MochaFn;
    const title = typeof titleOrFn === 'string' ? titleOrFn : `"${kind}" hook`;
    const timeout = typeof maybeFn === 'number' ? maybeFn : maybeTimeout;
    const suite = currentSuite;
    // vitest parses a hook's parameter list for fixtures and rejects plain
    // named parameters, so take the per-test context from `arguments`.
    (register as any)(async function () {
      const ctx = perTest ? (arguments[0] as TestContext) : undefined;
      // *All hooks receive the suite (or file) task as their second argument.
      const suiteTask = perTest ? undefined : arguments[1];
      await runMochaStyle(fn, suite, `${kind} hook`, title, ctx, timeout, suiteTask);
      // Never hand vitest a return value: a function returned from a before*
      // hook would be registered as its teardown.
    });
  };
}

const it = Object.assign(makeIt('run'), {
  only: makeIt('only'),
  skip: makeIt('skip'),
  todo: (title: string) => vitest.it.todo(title)
});
const describe = Object.assign(makeDescribe('run'), {
  only: makeDescribe('only'),
  skip: makeDescribe('skip'),
  todo: (title: string) => vitest.describe.todo(title)
});

export const mochaGlobals = {
  describe,
  context: describe,
  xdescribe: describe.skip,
  it,
  specify: it,
  xit: it.skip,
  before: makeHook(vitest.beforeAll, 'before all', false),
  after: makeHook(vitest.afterAll, 'after all', false),
  beforeEach: makeHook(vitest.beforeEach, 'before each', true),
  afterEach: makeHook(vitest.afterEach, 'after each', true)
};

export function installMochaGlobals(target: any = globalThis) {
  Object.assign(target, mochaGlobals);
}
