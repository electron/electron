import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';

import { ifdescribe, isTestingBindingAvailable, startRemoteControlApp } from './lib/spec-helpers';

describe('cpp heap', () => {
  describe('app module', () => {
    it('should not allocate on every require', async () => {
      const { remotely } = await startRemoteControlApp();
      const [usedBefore, usedAfter] = await remotely(async () => {
        const { getCppHeapStatistics } = require('node:v8');
        const heapStatsBefore = getCppHeapStatistics('brief');
        {
          const { app } = require('electron');
          console.log(app.name);
        }
        {
          const { app } = require('electron');
          console.log(app.dock);
        }
        const heapStatsAfter = getCppHeapStatistics('brief');
        return [heapStatsBefore.used_size_bytes, heapStatsAfter.used_size_bytes];
      });
      expect(usedBefore).to.be.equal(usedAfter);
    });

    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(async (heap: string, snapshotHelper: string) => {
        const { recordState } = require(heap);
        const { containsRetainingPath } = require(snapshotHelper);
        const state = recordState();
        return containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / App']);
      }, path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
      path.join(__dirname, 'lib', 'heapsnapshot-helpers.js'));
      expect(result).to.equal(true);
    });
  });

  describe('session module', () => {
    it('does not crash on exit with live session wrappers', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(async () => {
        const { app, session } = require('electron');

        const sessions = [
          session.defaultSession,
          session.fromPartition('cppheap-exit'),
          session.fromPartition('persist:cppheap-exit-persist')
        ];

        // We want to test GC on shutdown, so add a global reference
        // to these sessions to prevent pre-shutdown GC.
        (globalThis as any).sessionRefs = sessions;

        // We want to test CppGC-traced references during shutdown.
        // The CppGC-managed cookies will do that; but since they're
        // lazy-created, access them here to ensure they're live.
        sessions.forEach(ses => ses.cookies);

        setTimeout(() => app.quit());
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
    });

    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(async (heap: string, snapshotHelper: string) => {
        const { session, BrowserWindow } = require('electron');
        const { once } = require('node:events');
        const assert = require('node:assert');
        const { recordState } = require(heap);
        const { containsRetainingPath } = require(snapshotHelper);
        const session1 = session.defaultSession;
        console.log(session1.getStoragePath());
        const session2 = session.fromPartition('cppheap1');
        const session3 = session.fromPartition('cppheap1');
        const session4 = session.fromPartition('cppheap2');
        console.log(session2.cookies);
        assert.strictEqual(session2, session3);
        assert.notStrictEqual(session2, session4);
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            session: session.fromPartition('cppheap1')
          }
        });
        await w.loadURL('about:blank');
        const state = recordState();
        const isClosed = once(w, 'closed');
        w.destroy();
        await isClosed;
        const numSessions = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / Session'], {
          occurrences: 4
        });
        const canTraceJSReferences = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / Session', 'Cookies']);
        return numSessions && canTraceJSReferences;
      }, path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
      path.join(__dirname, 'lib', 'heapsnapshot-helpers.js'));
      expect(result).to.equal(true);
    });
  });

  ifdescribe(isTestingBindingAvailable())('SafeV8Function callback conversion', () => {
    const gcTestArgv = ['--js-flags=--expose-gc'];

    it('retains repeating callback while held, allows multiple invocations, then releases', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise(resolve => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callCount = 0;
        let repeating: any = () => { callCount++; };
        const repeatingWeakRef = new WeakRef(repeating);
        testingBinding.holdRepeatingCallbackForTesting(repeating);
        repeating = null;

        const invoked0 = testingBinding.invokeHeldRepeatingCallbackForTesting();
        const invoked1 = testingBinding.invokeHeldRepeatingCallbackForTesting();
        const invoked2 = testingBinding.invokeHeldRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => repeatingWeakRef.deref() === undefined);

        return { invoked0, invoked1, invoked2, callCount, releasedAfterClear };
      });

      expect(result.invoked0).to.equal(true, 'first invocation should succeed');
      expect(result.invoked1).to.equal(true, 'second invocation should succeed');
      expect(result.invoked2).to.equal(true, 'third invocation should succeed');
      expect(result.callCount).to.equal(3, 'callback should have been called 3 times');
      expect(result.releasedAfterClear).to.equal(true, 'callback should be released after clear');
    });

    it('consumes once callback on first invoke and releases it', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise(resolve => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callCount = 0;
        let once: any = () => { callCount++; };
        const onceWeakRef = new WeakRef(once);
        testingBinding.holdOnceCallbackForTesting(once);
        once = null;

        const first = testingBinding.invokeHeldOnceCallbackForTesting();
        const second = testingBinding.invokeHeldOnceCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const released = await waitForGC(() => onceWeakRef.deref() === undefined);

        return { first, second, callCount, released };
      });

      expect(result.first).to.equal(true, 'first invoke should succeed');
      expect(result.second).to.equal(false, 'second invoke should fail (consumed)');
      expect(result.callCount).to.equal(1, 'callback should have been called once');
      expect(result.released).to.equal(true, 'callback should be released after consume + clear');
    });

    it('releases replaced repeating callback while keeping latest callback alive', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise(resolve => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callbackA: any = () => {};
        const weakA = new WeakRef(callbackA);
        testingBinding.holdRepeatingCallbackForTesting(callbackA);
        callbackA = null;

        let callbackB: any = () => {};
        const weakB = new WeakRef(callbackB);
        testingBinding.holdRepeatingCallbackForTesting(callbackB);
        callbackB = null;

        const releasedA = await waitForGC(() => weakA.deref() === undefined);

        testingBinding.clearHeldCallbacksForTesting();
        const releasedB = await waitForGC(() => weakB.deref() === undefined);

        return { releasedA, releasedB };
      });

      expect(result.releasedA).to.equal(true, 'replaced callback A should be released');
      expect(result.releasedB).to.equal(true, 'callback B should be released after clear');
    });

    it('keeps callback alive while copied holder exists and releases after all copies clear', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise(resolve => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let repeating: any = () => {};
        const weakRef = new WeakRef(repeating);
        testingBinding.holdRepeatingCallbackForTesting(repeating);
        repeating = null;

        const copied = testingBinding.copyHeldRepeatingCallbackForTesting();
        const countAfterCopy = testingBinding.getHeldRepeatingCallbackCountForTesting();
        testingBinding.clearPrimaryHeldRepeatingCallbackForTesting();

        const invokedViaCopy = testingBinding.invokeCopiedRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => weakRef.deref() === undefined);

        return { copied, countAfterCopy, invokedViaCopy, releasedAfterClear };
      });

      expect(result.copied).to.equal(true, 'copy should succeed');
      expect(result.countAfterCopy).to.equal(2, 'should have 2 holders after copy');
      expect(result.invokedViaCopy).to.equal(true, 'invoke via copy should succeed');
      expect(result.releasedAfterClear).to.equal(true, 'callback should be released after all copies clear');
    });

    it('does not leak repeating callback when callback throws during invocation', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise(resolve => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let throwing: any = () => { throw new Error('expected test throw'); };
        const weakRef = new WeakRef(throwing);
        testingBinding.holdRepeatingCallbackForTesting(throwing);
        throwing = null;

        const invokeResult = testingBinding.invokeHeldRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => weakRef.deref() === undefined);

        return { invokeResult, releasedAfterClear };
      });

      expect(result.invokeResult).to.equal(false, 'invoke should fail (callback throws)');
      expect(result.releasedAfterClear).to.equal(true, 'throwing callback should be released after clear');
    });
  });

  describe('internal event', () => {
    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(async (heap: string, snapshotHelper: string) => {
        const { BrowserWindow } = require('electron');
        const { once } = require('node:events');
        const { recordState } = require(heap);
        const { containsRetainingPath } = require(snapshotHelper);

        const w = new BrowserWindow({
          show: false
        });
        await w.loadURL('about:blank');
        const state = recordState();
        const isClosed = once(w, 'closed');
        w.destroy();
        await isClosed;
        const eventNativeStackReference = containsRetainingPath(state.snapshot, ['C++ native stack roots', 'Electron / Event']);
        const noPersistentReference = !containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / Event']);
        return eventNativeStackReference && noPersistentReference;
      }, path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
      path.join(__dirname, 'lib', 'heapsnapshot-helpers.js'));
      expect(result).to.equal(true);
    });
  });
});
