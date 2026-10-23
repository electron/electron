import { afterAll, afterEach, beforeAll, describe, expect, it, vi } from 'vitest';

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

  describe('when a defer()-ed cleanup outlasts the hook timeout', () => {
    const ran: string[] = [];
    let reported: string[] | undefined;

    // A short hook timeout keeps this fast; defer()-ed cleanup reads it when it runs.
    beforeAll(() => {
      vi.setConfig({ hookTimeout: 100 });
    });
    afterAll(() => {
      vi.resetConfig();
    });

    afterEach(() => {
      ran.push('afterEach');
    });

    it.fails('fails the test with a timeout', { retry: 0 }, ({ onTestFailed }) => {
      onTestFailed(({ task }) => {
        reported = task.result?.errors?.map((error) => error.message);
      });
      defer(() => {
        ran.push('first deferred');
      });
      defer(() => {
        ran.push('second deferred');
        // Settles well after the 100 ms hook timeout,
        // so a runner without the timeout fails this test instead of hanging.
        return new Promise<void>((resolve) => setTimeout(resolve, 5000));
      });
    });

    // Reads what the test above left behind, so it must run after it.
    it('still runs every later cleanup and hook', () => {
      expect(ran).toEqual(['second deferred', 'first deferred', 'afterEach']);
      expect(reported).toEqual(['defer()-ed cleanup timed out in 100ms']);
    });
  });
});
