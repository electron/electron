import { utilityProcess } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';

import { deferKillUtilityProcess } from './spec-helpers.ts';

const fixturePath = path.resolve(import.meta.dirname, '..', 'fixtures', 'api', 'utility-process', 'api-net-spec.js');

// Waits for the result the api-net-spec.js fixture posts, then for the child to exit.
// Both listeners are attached before this returns, so call it before posting work.
// Rejects as soon as the child exits without having posted a result,
// instead of leaving the test to wait for its timeout.
export async function waitForUtilityResult(child: NodeJS.EventEmitter) {
  const exit = once(child, 'exit');
  const [data] = await Promise.race([
    once(child, 'message'),
    exit.then(([code]) => {
      throw new Error(`utility process exited (${code}) before posting a result`);
    })
  ]);
  const [code] = await exit;
  return { data, code };
}

// Runs a test body in the api-net-spec.js fixture's utility process and asserts that it passed.
// The body is sent as source text; see the fixture for what it can refer to.
export async function runInUtilityProcess(fn: Function = () => {}, args?: { [key: string]: any }) {
  const child = utilityProcess.fork(fixturePath, [], {
    execArgv: ['--expose-gc']
  });
  deferKillUtilityProcess(child);
  const result = waitForUtilityResult(child);
  child.postMessage({ fn: `(${fn})()`, args });
  const { data, code } = await result;
  expect(data.ok).to.be.true(data.message);
  expect(code).to.equal(0);
}
