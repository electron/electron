import { afterEach, describe, expect, it } from 'vitest';

import { defer } from './lib/spec-helpers.ts';

// Behaviour that spec/vitest/runner.ts adds to vitest.
describe('spec runner', () => {
  describe('when afterEach hooks and defer()-ed cleanup throw', () => {
    const ran: string[] = [];
    let reported: string[] | undefined;

    afterEach(() => {
      ran.push('outer afterEach');
    });

    describe('in a nested suite', () => {
      afterEach(() => {
        ran.push('first afterEach');
        throw new Error('afterEach failure');
      });
      afterEach(() => {
        ran.push('second afterEach');
      });

      it.fails('fails the test', { retry: 0 }, ({ onTestFailed }) => {
        onTestFailed(({ task }) => {
          reported = task.result?.errors?.map((error) => error.message);
        });
        defer(() => {
          ran.push('first deferred');
        });
        defer(() => {
          ran.push('second deferred');
          throw new Error('second cleanup failure');
        });
        defer(() => {
          ran.push('third deferred');
          throw new Error('third cleanup failure');
        });
      });
    });

    // Reads what the test above left behind, so it must run after it.
    it('still runs every later cleanup and hook, and reports every error', () => {
      expect(ran).toEqual([
        'third deferred',
        'second deferred',
        'first deferred',
        'first afterEach',
        'second afterEach',
        'outer afterEach'
      ]);
      expect(reported).toEqual(['third cleanup failure', 'second cleanup failure', 'afterEach failure']);
    });
  });
});
