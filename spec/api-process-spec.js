const { ipcRenderer } = require('electron');
const fs = require('fs');
const path = require('path');

const { expect } = require('chai');

describe('process module', () => {
  describe('process.getCreationTime()', () => {
    it('returns a creation time', () => {
      const creationTime = process.getCreationTime();
      expect(creationTime).to.be.a('number').and.be.at.least(0);
    });
  });

  describe('process.getCPUUsage()', () => {
    it('returns a cpu usage object', () => {
      const cpuUsage = process.getCPUUsage();
      expect(cpuUsage.percentCPUUsage).to.be.a('number');
      expect(cpuUsage.idleWakeupsPerSecond).to.be.a('number');
    });
  });

  describe('process.getIOCounters()', () => {
    before(function () {
      if (process.platform === 'darwin') {
        this.skip();
      }
    });

    it('returns an io counters object', () => {
      const ioCounters = process.getIOCounters();
      expect(ioCounters.readOperationCount).to.be.a('number');
      expect(ioCounters.writeOperationCount).to.be.a('number');
      expect(ioCounters.otherOperationCount).to.be.a('number');
      expect(ioCounters.readTransferCount).to.be.a('number');
      expect(ioCounters.writeTransferCount).to.be.a('number');
      expect(ioCounters.otherTransferCount).to.be.a('number');
    });
  });

  describe('process.getBlinkMemoryInfo()', () => {
    it('returns blink memory information object', () => {
      const heapStats = process.getBlinkMemoryInfo();
      expect(heapStats.allocated).to.be.a('number');
      expect(heapStats.total).to.be.a('number');
    });
  });

  describe('process.getProcessMemoryInfo()', async () => {
    it('resolves promise successfully with valid data', async () => {
      const memoryInfo = await process.getProcessMemoryInfo();
      expect(memoryInfo).to.be.an('object');
      if (process.platform === 'linux' || process.platform === 'windows') {
        expect(memoryInfo.residentSet).to.be.a('number').greaterThan(0);
      }
      expect(memoryInfo.private).to.be.a('number').greaterThan(0);
      // Shared bytes can be zero
      expect(memoryInfo.shared).to.be.a('number').greaterThan(-1);
    });
  });

  describe('process.getSystemMemoryInfo()', () => {
    it('returns system memory info object', () => {
      const systemMemoryInfo = process.getSystemMemoryInfo();
      expect(systemMemoryInfo.free).to.be.a('number');
      expect(systemMemoryInfo.total).to.be.a('number');
    });
  });

  describe('process.getSystemVersion()', () => {
    it('returns a string', () => {
      expect(process.getSystemVersion()).to.be.a('string');
    });
  });

  describe('process.getHeapStatistics()', () => {
    it('returns heap statistics object', () => {
      const heapStats = process.getHeapStatistics();
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
    it('returns true on success', async () => {
      const filePath = path.join(await ipcRenderer.invoke('get-temp-dir'), 'test.heapsnapshot');

      const cleanup = () => {
        try {
          fs.unlinkSync(filePath);
        } catch (e) {
          // ignore error
        }
      };

      try {
        const success = process.takeHeapSnapshot(filePath);
        expect(success).to.be.true();
        const stats = fs.statSync(filePath);
        expect(stats.size).not.to.be.equal(0);
      } finally {
        cleanup();
      }
    });

    it('returns false on failure', () => {
      const success = process.takeHeapSnapshot('');
      expect(success).to.be.false();
    });
  });

  describe('process.contextId', () => {
    it('is a string', () => {
      expect(process.contextId).to.be.a('string');
    });
  });
});
