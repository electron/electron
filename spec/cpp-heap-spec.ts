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
      const [nodeCount, hasPersistentParent] = await remotely(async (heap: string) => {
        const { recordState } = require(heap);
        const state = recordState();
        const rootNodes = state.snapshot.filter(
          (node: any) => node.name === 'Electron / App' && node.type !== 'string');
        const hasParent = rootNodes.some((node: any) => node.incomingEdges.some(
          (edge: any) => {
            return edge.type === 'element' && edge.from.name === 'C++ Persistent roots';
          }));
        return [rootNodes.length, hasParent];
      }, path.join(__dirname, '../../third_party/electron_node/test/common/heap'));
      expect(nodeCount).to.equal(1);
      expect(hasPersistentParent).to.be.true();
    });
  });
});
