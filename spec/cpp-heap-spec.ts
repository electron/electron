import { expect } from 'chai';

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
