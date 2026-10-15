import { describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';

import { ifdescribe, isTestingBindingAvailable, startRemoteControlApp } from './lib/spec-helpers.ts';

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
      expect(usedBefore).toBe(usedAfter);
    });

    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const state = recordState();
          return containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / App']);
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).toBe(true);
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
        sessions.forEach((ses) => ses.cookies);

        setTimeout(() => app.quit());
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
    });

    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
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
          const canTraceJSReferences = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / Session',
            'Electron / Cookies'
          ]);
          return numSessions && canTraceJSReferences;
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).toBe(true);
    });
  });

  describe('cookies module', () => {
    it('should return the same Cookies instance for the same session', async () => {
      const { remotely } = await startRemoteControlApp();
      const result = await remotely(async () => {
        const { session } = require('electron');
        const ses = session.fromPartition('cookies-identity');
        const cookies1 = ses.cookies;
        const cookies2 = ses.cookies;
        return cookies1 === cookies2;
      });
      expect(result, 'cookies getter should return the same instance').toBe(true);
    });

    it('should survive GC when only referenced through session', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { session } = require('electron');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          // Access cookies to create the C++ object, then drop the JS reference.
          const ses = session.defaultSession;
          let cookies: any = ses.cookies;
          await cookies.get({});
          cookies = null;

          // Force GC — the Cookies object should survive because
          // Session traces it via cppgc::Member.
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const stillAlive = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / Session',
            'Electron / Cookies'
          ]);
          return stillAlive;
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result, 'Cookies should survive GC when traced from Session').toBe(true);
    });
  });

  ifdescribe(process.platform === 'darwin')('autoUpdater module', () => {
    it('is retained after garbage collection', async () => {
      const rc = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await rc.remotely(async () => {
        const { autoUpdater } = require('electron');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
        const wr = new WeakRef(autoUpdater);
        for (let i = 0; i < 10; i++) {
          await new Promise((resolve) => setTimeout(resolve, 0));
          v8Util.requestGarbageCollectionForTesting();
        }
        return {
          retained: wr.deref() !== undefined,
          functional: typeof autoUpdater.getFeedURL() === 'string'
        };
      });
      expect(result.retained, 'autoUpdater should survive GC').toBe(true);
      expect(result.functional, 'autoUpdater should still be functional after GC').toBe(true);
    });

    it('should record as node in heap snapshot', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string) => {
          const { autoUpdater, app } = require('electron');
          const { recordState } = require(heap);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
          console.log(autoUpdater.getFeedURL());
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          const state = recordState();
          const nodes = state.snapshot.filter((node: any) => node.name === 'Electron / AutoUpdater');
          const found = nodes.length > 0;
          const noDuplicates = nodes.length === 1;
          setTimeout(() => app.quit());
          return { found, noDuplicates };
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
      expect(result.found, 'AutoUpdater should be in snapshot (held by JS module)').toBe(true);
      expect(result.noDuplicates, 'should have exactly one AutoUpdater instance').toBe(true);
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
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callCount = 0;
        let repeating: any = () => {
          callCount++;
        };
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

      expect(result.invoked0, 'first invocation should succeed').toBe(true);
      expect(result.invoked1, 'second invocation should succeed').toBe(true);
      expect(result.invoked2, 'third invocation should succeed').toBe(true);
      expect(result.callCount, 'callback should have been called 3 times').toBe(3);
      expect(result.releasedAfterClear, 'callback should be released after clear').toBe(true);
    });

    it('consumes once callback on first invoke and releases it', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callCount = 0;
        let once: any = () => {
          callCount++;
        };
        const onceWeakRef = new WeakRef(once);
        testingBinding.holdOnceCallbackForTesting(once);
        once = null;

        const first = testingBinding.invokeHeldOnceCallbackForTesting();
        const second = testingBinding.invokeHeldOnceCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const released = await waitForGC(() => onceWeakRef.deref() === undefined);

        return { first, second, callCount, released };
      });

      expect(result.first, 'first invoke should succeed').toBe(true);
      expect(result.second, 'second invoke should fail (consumed)').toBe(false);
      expect(result.callCount, 'callback should have been called once').toBe(1);
      expect(result.released, 'callback should be released after consume + clear').toBe(true);
    });

    it('releases replaced repeating callback while keeping latest callback alive', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
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

      expect(result.releasedA, 'replaced callback A should be released').toBe(true);
      expect(result.releasedB, 'callback B should be released after clear').toBe(true);
    });

    it('keeps callback alive while copied holder exists and releases after all copies clear', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
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

      expect(result.copied, 'copy should succeed').toBe(true);
      expect(result.countAfterCopy, 'should have 2 holders after copy').toBe(2);
      expect(result.invokedViaCopy, 'invoke via copy should succeed').toBe(true);
      expect(result.releasedAfterClear, 'callback should be released after all copies clear').toBe(true);
    });

    it('does not leak repeating callback when callback throws during invocation', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let throwing: any = () => {
          throw new Error('expected test throw');
        };
        const weakRef = new WeakRef(throwing);
        testingBinding.holdRepeatingCallbackForTesting(throwing);
        throwing = null;

        const invokeResult = testingBinding.invokeHeldRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => weakRef.deref() === undefined);

        return { invokeResult, releasedAfterClear };
      });

      expect(result.invokeResult, 'invoke should fail (callback throws)').toBe(false);
      expect(result.releasedAfterClear, 'throwing callback should be released after clear').toBe(true);
    });
  });

  describe('menu module', () => {
    // Regression test for https://github.com/electron/electron/issues/50791
    it('should not leak when rebuilding application menu', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(async () => {
        const { Menu } = require('electron');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
        const { getCppHeapStatistics } = require('node:v8');

        function buildLargeMenu() {
          return Menu.buildFromTemplate(
            Array.from({ length: 10 }, (_, i) => ({
              label: `Menu ${i}`,
              submenu: Array.from({ length: 20 }, (_, j) => ({
                label: `Item ${i}-${j}`,
                click: () => {}
              }))
            }))
          );
        }

        async function rebuildAndMeasure(n: number) {
          for (let i = 0; i < n; i++) {
            Menu.setApplicationMenu(buildLargeMenu());
          }
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          return getCppHeapStatistics('brief').used_size_bytes;
        }

        await rebuildAndMeasure(50);
        const after1 = await rebuildAndMeasure(50);
        const after2 = await rebuildAndMeasure(50);
        return { after1, after2 };
      });

      const growth = result.after2 - result.after1;
      expect(
        growth,
        `C++ heap grew by ${growth} bytes between two identical rounds of 100 menu rebuilds — likely a leak`
      ).toBeLessThanOrEqual(result.after1 * 0.1);
    });
  });

  describe('internal event', () => {
    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
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
          const eventNativeStackReference = containsRetainingPath(state.snapshot, [
            'C++ native stack roots',
            'Electron / Event'
          ]);
          const noPersistentReference = !containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / Event'
          ]);
          return eventNativeStackReference && noPersistentReference;
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).toBe(true);
    });
  });

  describe('powerMonitor module', () => {
    it('should retain native PowerMonitor via JS module reference', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string) => {
          const { powerMonitor, app } = require('electron');
          const { recordState } = require(heap);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          // Register a listener to trigger native PowerMonitor creation.
          const listener = () => {};
          powerMonitor.on('suspend', listener);

          // Add and remove several listeners to exercise the path,
          // then GC to ensure no duplicate native objects are created.
          for (let i = 0; i < 5; i++) {
            const tmp = () => {};
            powerMonitor.on('resume', tmp);
            powerMonitor.removeListener('resume', tmp);
          }

          v8Util.requestGarbageCollectionForTesting();

          const state = recordState();
          const nodes = state.snapshot.filter((node: any) => node.name === 'Electron / PowerMonitor');
          const found = nodes.length > 0;
          const noDuplicates = nodes.length === 1;

          setTimeout(() => app.quit());
          return { found, noDuplicates };
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
      expect(result.found, 'PowerMonitor should be in snapshot (held by JS module)').toBe(true);
      expect(result.noDuplicates, 'should have exactly one PowerMonitor instance').toBe(true);
    });
  });

  describe('utilityProcess module', () => {
    it('should appear in heap snapshot while process is running', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string, fixturePath: string) => {
          const { utilityProcess } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);

          const child = utilityProcess.fork(fixturePath);
          await once(child, 'spawn');

          const state = recordState();
          const found = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / UtilityProcess']);

          child.kill();
          await once(child, 'exit');
          return found;
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js'),
        path.join(import.meta.dirname, 'fixtures/api/utility-process/endless.js')
      );
      expect(result).toBe(true);
    });

    it('should be released from heap snapshot after process exits', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string, fixturePath: string) => {
          const { utilityProcess } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          let child: any = utilityProcess.fork(fixturePath);
          await once(child, 'exit');
          child = null;

          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const found = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / UtilityProcess']);
          return !found;
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js'),
        path.join(import.meta.dirname, 'fixtures/api/utility-process/empty.js')
      );
      expect(result, 'UtilityProcess should be released after exit and GC').toBe(true);
    });

    it('should survive GC when JS reference is dropped but process is still running', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string, snapshotHelper: string, fixturePath: string) => {
          const { utilityProcess, app } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          let child: any = utilityProcess.fork(fixturePath);
          await once(child, 'spawn');
          child = null;

          // Force GC - the process should still be alive because
          // SelfKeepAlive roots the C++ wrapper.
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const stillAlive = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / UtilityProcess'
          ]);

          setTimeout(() => app.quit());
          return stillAlive;
        },
        path.join(import.meta.dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(import.meta.dirname, 'lib', 'heapsnapshot-helpers.js'),
        path.join(import.meta.dirname, 'fixtures/api/utility-process/endless.js')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
      expect(result, 'UtilityProcess should survive GC while process is running').toBe(true);
    });

    it('should not leak when forking multiple processes', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(
        async (fixturePath: string) => {
          const { utilityProcess } = require('electron');
          const { once } = require('node:events');
          const { getCppHeapStatistics } = require('node:v8');
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          async function forkAndWait() {
            const child = utilityProcess.fork(fixturePath);
            await once(child, 'exit');
          }

          async function measure(n: number) {
            for (let i = 0; i < n; i++) {
              await forkAndWait();
            }
            for (let i = 0; i < 10; i++) {
              await new Promise((resolve) => setTimeout(resolve, 0));
              v8Util.requestGarbageCollectionForTesting();
            }
            return getCppHeapStatistics('brief').used_size_bytes;
          }

          await measure(5);
          const after1 = await measure(10);
          const after2 = await measure(10);
          return { after1, after2 };
        },
        path.join(import.meta.dirname, 'fixtures/api/utility-process/empty.js')
      );

      const growth = result.after2 - result.after1;
      expect(growth, `C++ heap grew by ${growth} bytes between rounds - likely a leak`).toBeLessThanOrEqual(
        result.after1 * 0.1
      );
    });
  });

  ifdescribe(isTestingBindingAvailable())('gin_helper::Promise cppgc', () => {
    it('does not crash on exit with a live PromiseBase', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(async () => {
        const { app } = require('electron');
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');

        // Create a gin_helper::Promise<void> and hold it in a static C++
        // variable. It will outlive the isolate and be destroyed during
        // static destruction - this must not crash.
        testingBinding.holdPromiseForTesting();

        setTimeout(() => app.quit());
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
    });

    it('PromiseHandle should not leak', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(async () => {
        const { getCppHeapStatistics } = require('node:v8');
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');

        // Count PromiseHandles in the C++ heap's per-type statistics.
        // These include any handle not yet reclaimed, garbage too,
        // so the test collects before each count,
        // and the count then reflects the handles live at that point.
        // A heap snapshot shows the same objects,
        // but taking and parsing three snapshots of the main process
        // can overrun the test timeout on slow CI machines.
        const countPromiseHandles = () => {
          const { type_names: typeNames, space_statistics: spaces } = getCppHeapStatistics('detailed');
          let count = 0;
          for (const space of spaces) {
            for (const page of space.page_stats) {
              page.object_statistics.forEach((entry: any, index: number) => {
                if (typeNames[index] === 'Electron / PromiseHandle') count += entry.object_count;
              });
            }
          }
          return count;
        };

        // Collect from a fresh task without scanning the native stack,
        // so a stale pointer cannot keep a released PromiseHandle alive.
        // This uses V8's gc() rather than v8Util's async collection,
        // whose returned promise would itself leave a PromiseHandle to count.
        // Several rounds also reclaim objects that only become unreachable
        // after an earlier round's weak callbacks have run.
        const gc = async () => {
          for (let i = 0; i < 3; i++) {
            await globalThis.gc!({ type: 'major', execution: 'async' });
          }
        };

        // The first count is taken before any collection,
        // so start from a collected heap that holds no garbage for gc() to remove.
        await gc();
        testingBinding.holdPromiseForTesting();
        const beforeGC = countPromiseHandles();
        await gc();
        const afterGC = countPromiseHandles();

        testingBinding.clearHeldPromiseForTesting();
        await gc();
        const afterClear = countPromiseHandles();

        return { beforeGC, afterGC, afterClear };
      });

      expect(result.afterGC, 'held PromiseHandle must survive GC').toBeGreaterThanOrEqual(result.beforeGC);
      expect(result.afterClear, 'clearing should release the PromiseHandle').toBeLessThan(result.afterGC);
    });
  });
});
