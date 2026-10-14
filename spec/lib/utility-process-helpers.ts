import { utilityProcess } from 'electron/main';

import { expect } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';

import { deferKillUtilityProcess } from './spec-helpers.ts';

const fixturePath = path.resolve(import.meta.dirname, '..', 'fixtures', 'api', 'utility-process', 'api-net-spec.js');

// Waits for the result the api-net-spec.js fixture posts, acknowledges it, then
// waits for the child to exit. The fixture only exits once it has the
// acknowledgement, so its result can't be overtaken by (or lost to) its exit.
// Both listeners are attached before this returns, so call it before posting work.
// Rejects as soon as the child exits without having posted a result,
// instead of leaving the test to wait for its timeout.
export async function waitForUtilityResult(child: NodeJS.EventEmitter & { postMessage?(message: unknown): void }) {
  const exit = once(child, 'exit');
  const [data] = await Promise.race([
    once(child, 'message'),
    exit.then(([code]) => {
      throw new Error(`utility process exited (${code}) before posting a result`);
    })
  ]);
  child.postMessage?.('ack');
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
  expect(data.ok, data.message).toBe(true);
  expect(code).toBe(0);
}
