// Logs every test that needed a retry. The default reporter only prints a
// test (and its "retry xN" note) when it fails or is slower than
// slowTestThreshold, so a fast test that passes on a retry leaves no trace in
// the CI log.

import * as path from 'node:path';

import type { Reporter, TestCase, Vitest } from 'vitest/node';

export class RetryReporter implements Reporter {
  readonly #baseDir: string;
  #vitest: Vitest | undefined;
  #retried = 0;

  constructor(specDir: string) {
    this.#baseDir = path.resolve(specDir, '..');
  }

  onInit(vitest: Vitest) {
    this.#vitest = vitest;
  }

  onTestCaseResult(testCase: TestCase) {
    const diagnostic = testCase.diagnostic();
    if (!diagnostic || diagnostic.retryCount === 0) return;
    this.#retried++;
    const file = path.relative(this.#baseDir, testCase.module.moduleId).split(path.sep).join('/');
    const state = testCase.result().state === 'passed' ? 'pass' : 'fail';
    this.#log(
      `[retried] ${testCase.fullName} (${file}) retry x${diagnostic.retryCount}, ${Math.round(diagnostic.duration)}ms, ${state}`
    );
  }

  onTestRunEnd() {
    if (this.#retried > 0) {
      this.#log(`[retried] ${this.#retried} test(s) needed a retry`);
    }
  }

  // Through vitest's logger so the line does not tear the default reporter's
  // live summary on a terminal.
  #log(message: string) {
    if (this.#vitest) this.#vitest.logger.log(message);
    else console.log(message);
  }
}
