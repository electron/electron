import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';

import { startRemoteControlApp } from './lib/spec-helpers';

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

  describe('menu module', () => {
    // Regression test for https://github.com/electron/electron/issues/50791
    it('should not leak when rebuilding application menu', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(async () => {
        const { Menu } = require('electron');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
        const { getCppHeapStatistics } = require('node:v8');

        function buildLargeMenu () {
          return Menu.buildFromTemplate(
            Array.from({ length: 20 }, (_, i) => ({
              label: `Menu ${i}`,
              submenu: Array.from({ length: 50 }, (_, j) => ({
                label: `Item ${i}-${j}`,
                click: () => {}
              }))
            }))
          );
        }

        async function rebuildAndMeasure (n: number) {
          for (let i = 0; i < n; i++) {
            Menu.setApplicationMenu(buildLargeMenu());
          }
          for (let i = 0; i < 10; i++) {
            await new Promise(resolve => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          return getCppHeapStatistics('brief').used_size_bytes;
        }

        await rebuildAndMeasure(100);
        const after1 = await rebuildAndMeasure(100);
        const after2 = await rebuildAndMeasure(100);
        return { after1, after2 };
      });

      const growth = result.after2 - result.after1;
      expect(growth).to.be.at.most(
        result.after1 * 0.1,
        `C++ heap grew by ${growth} bytes between two identical rounds of 100 menu rebuilds — likely a leak`
      );
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
