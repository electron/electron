import { BrowserWindow } from 'electron';
import { app } from 'electron/main';

import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as path from 'node:path';

import { defer, ifdescribe } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('process module', () => {
  function generateSpecs(
    invoke: <T extends (...args: any[]) => any>(fn: T, ...args: Parameters<T>) => Promise<ReturnType<T>>
  ) {
    describe('process.getCreationTime()', () => {
      it('returns a creation time', async () => {
        const creationTime = await invoke(() => process.getCreationTime());
        expect(creationTime).toBeTypeOf('number');
        expect(creationTime).toBeGreaterThanOrEqual(0);
      });
    });

    describe('process.getCPUUsage()', () => {
      it('returns a cpu usage object', async () => {
        const cpuUsage = await invoke(() => process.getCPUUsage());
        expect(cpuUsage.percentCPUUsage).toBeTypeOf('number');
        expect(cpuUsage.cumulativeCPUUsage).toBeTypeOf('number');
        expect(cpuUsage.idleWakeupsPerSecond).toBeTypeOf('number');
      });
    });

    describe('process.getBlinkMemoryInfo()', () => {
      it('returns blink memory information object', async () => {
        const heapStats = await invoke(() => process.getBlinkMemoryInfo());
        expect(heapStats.allocated).toBeTypeOf('number');
        expect(heapStats.total).toBeTypeOf('number');
      });
    });

    describe('process.getProcessMemoryInfo()', () => {
      it('resolves promise successfully with valid data', async () => {
        const memoryInfo = await invoke(() => process.getProcessMemoryInfo());
        expect(memoryInfo).toBeTypeOf('object');
        if (process.platform === 'linux' || process.platform === 'win32') {
          expect(memoryInfo.residentSet).toBeTypeOf('number');
          expect(memoryInfo.residentSet).toBeGreaterThan(0);
        }
        expect(memoryInfo.private).toBeTypeOf('number');
        expect(memoryInfo.private).toBeGreaterThan(0);
        // Shared bytes can be zero
        expect(memoryInfo.shared).toBeTypeOf('number');
        expect(memoryInfo.shared).toBeGreaterThan(-1);
      });
    });

    describe('process.getSystemMemoryInfo()', () => {
      it('returns system memory info object', async () => {
        const systemMemoryInfo = await invoke(() => process.getSystemMemoryInfo());
        expect(systemMemoryInfo.free).toBeTypeOf('number');
        expect(systemMemoryInfo.total).toBeTypeOf('number');
        if (process.platform === 'linux') {
          expect(systemMemoryInfo.available).toBeTypeOf('number');
          expect(systemMemoryInfo.available).toBeGreaterThan(0);
        }
      });
    });

    describe('process.getSystemVersion()', () => {
      it('returns a string', async () => {
        const systemVersion = await invoke(() => process.getSystemVersion());
        expect(systemVersion).toBeTypeOf('string');
      });
    });

    describe('process.getHeapStatistics()', () => {
      it('returns heap statistics object', async () => {
        const heapStats = await invoke(() => process.getHeapStatistics());
        expect(heapStats.totalHeapSize).toBeTypeOf('number');
        expect(heapStats.totalHeapSizeExecutable).toBeTypeOf('number');
        expect(heapStats.totalPhysicalSize).toBeTypeOf('number');
        expect(heapStats.totalAvailableSize).toBeTypeOf('number');
        expect(heapStats.usedHeapSize).toBeTypeOf('number');
        expect(heapStats.heapSizeLimit).toBeTypeOf('number');
        expect(heapStats.mallocedMemory).toBeTypeOf('number');
        expect(heapStats.peakMallocedMemory).toBeTypeOf('number');
        expect(heapStats.doesZapGarbage).toBeTypeOf('boolean');
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
        expect(success).toBe(true);
        const stats = fs.statSync(filePath);
        expect(stats.size).not.toBe(0);
      });

      it('returns false on failure', async () => {
        const success = await invoke((filePath: string) => process.takeHeapSnapshot(filePath), '');
        expect(success).toBe(false);
      });
    });
  }

  describe('renderer process', () => {
    let w: BrowserWindow;
    beforeAll(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadURL('about:blank');
    });
    afterAll(closeAllWindows);

    generateSpecs((fn, ...args) => {
      const jsonArgs = args.map((value) => JSON.stringify(value)).join(',');
      return w.webContents.executeJavaScript(`(${fn.toString()})(${jsonArgs})`);
    });

    describe('process.contextId', () => {
      it('is a string', async () => {
        const contextId = await w.webContents.executeJavaScript('process.contextId');
        expect(contextId).toBeTypeOf('string');
      });
    });
  });

  describe('main process', () => {
    generateSpecs((fn, ...args) => fn(...args));
  });

  ifdescribe(process.platform === 'linux')('process.env', () => {
    it('can add variables while another thread reads the environment', async () => {
      const fixture = path.join(import.meta.dirname, 'fixtures', 'api', 'environ-write-race.js');
      const child = cp.spawn(process.execPath, [fixture], { stdio: ['ignore', 'pipe', 'inherit'] });
      let stdout = '';
      child.stdout.on('data', (chunk) => {
        stdout += chunk;
      });
      const [code, signal] = await once(child, 'close');
      expect({ code, signal, stdout: stdout.trim() }).toEqual({ code: 0, signal: null, stdout: 'done' });
    });
  });
});
