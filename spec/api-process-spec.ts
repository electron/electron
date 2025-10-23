import { BrowserWindow } from 'electron';
import { app } from 'electron/main';

import { expect } from 'chai';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { defer } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

describe('process module', () => {
  function generateSpecs (invoke: <T extends (...args: any[]) => any>(fn: T, ...args: Parameters<T>) => Promise<ReturnType<T>>) {
    describe('process.getCreationTime()', () => {
      it('returns a creation time', async () => {
        const creationTime = await invoke(() => process.getCreationTime());
        expect(creationTime).to.be.a('number').and.be.at.least(0);
      });
    });

    describe('process.getCPUUsage()', () => {
      it('returns a cpu usage object', async () => {
        const cpuUsage = await invoke(() => process.getCPUUsage());
        expect(cpuUsage.percentCPUUsage).to.be.a('number');
        expect(cpuUsage.cumulativeCPUUsage).to.be.a('number');
        expect(cpuUsage.idleWakeupsPerSecond).to.be.a('number');
      });
    });

    describe('process.getBlinkMemoryInfo()', () => {
      it('returns blink memory information object', async () => {
        const heapStats = await invoke(() => process.getBlinkMemoryInfo());
        expect(heapStats.allocated).to.be.a('number');
        expect(heapStats.total).to.be.a('number');
      });
    });

    describe('process.getProcessMemoryInfo()', () => {
      it('resolves promise successfully with valid data', async () => {
        const memoryInfo = await invoke(() => process.getProcessMemoryInfo());
        expect(memoryInfo).to.be.an('object');
        if (process.platform === 'linux' || process.platform === 'win32') {
          expect(memoryInfo.residentSet).to.be.a('number').greaterThan(0);
        }
        expect(memoryInfo.private).to.be.a('number').greaterThan(0);
        // Shared bytes can be zero
        expect(memoryInfo.shared).to.be.a('number').greaterThan(-1);
      });
    });

    describe('process.getSystemMemoryInfo()', () => {
      it('returns system memory info object', async () => {
        const systemMemoryInfo = await invoke(() => process.getSystemMemoryInfo());
        expect(systemMemoryInfo.free).to.be.a('number');
        expect(systemMemoryInfo.total).to.be.a('number');
      });
    });

    describe('process.getSystemVersion()', () => {
      it('returns a string', async () => {
        const systemVersion = await invoke(() => process.getSystemVersion());
        expect(systemVersion).to.be.a('string');
      });
    });

    describe('process.getHeapStatistics()', () => {
      it('returns heap statistics object', async () => {
        const heapStats = await invoke(() => process.getHeapStatistics());
        expect(heapStats.totalHeapSize).to.be.a('number');
        expect(heapStats.totalHeapSizeExecutable).to.be.a('number');
        expect(heapStats.totalPhysicalSize).to.be.a('number');
        expect(heapStats.totalAvailableSize).to.be.a('number');
        expect(heapStats.usedHeapSize).to.be.a('number');
        expect(heapStats.heapSizeLimit).to.be.a('number');
        expect(heapStats.mallocedMemory).to.be.a('number');
        expect(heapStats.peakMallocedMemory).to.be.a('number');
        expect(heapStats.doesZapGarbage).to.be.a('boolean');
      });
    });

    describe('process.takeHeapSnapshot()', () => {
      // DISABLED-FIXME(nornagon): this seems to take a really long time when run in the
      // main process, for unknown reasons.
      it('returns true on success', async () => {
        const filePath = path.join(app.getPath('temp'), 'test.heapsnapshot');
        defer(() => {
          try {
            fs.unlinkSync(filePath);
          } catch {
            // ignore error
          }
        });

        const success = await invoke((filePath: string) => process.takeHeapSnapshot(filePath), filePath);
        expect(success).to.be.true();
        const stats = fs.statSync(filePath);
        expect(stats.size).not.to.be.equal(0);
      });

      it('returns false on failure', async () => {
        const success = await invoke((filePath: string) => process.takeHeapSnapshot(filePath), '');
        expect(success).to.be.false();
      });
    });
  }

  describe('renderer process', () => {
    let w: BrowserWindow;
    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadURL('about:blank');
    });
    after(closeAllWindows);

    generateSpecs((fn, ...args) => {
      const jsonArgs = args.map(value => JSON.stringify(value)).join(',');
      return w.webContents.executeJavaScript(`(${fn.toString()})(${jsonArgs})`);
    });

    describe('process.contextId', () => {
      it('is a string', async () => {
        const contextId = await w.webContents.executeJavaScript('process.contextId');
        expect(contextId).to.be.a('string');
      });
    });
  });

  describe('main process', () => {
    generateSpecs((fn, ...args) => fn(...args));
  });
});
