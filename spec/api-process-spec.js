const assert = require('assert')

describe('process module', () => {
  describe('process.getCPUUsage()', () => {
    it('returns a cpu usage object', () => {
      const cpuUsage = process.getCPUUsage()
      assert.equal(typeof cpuUsage.percentCPUUsage, 'number')
      assert.equal(typeof cpuUsage.idleWakeupsPerSecond, 'number')
    })
  })

  describe('process.getIOCounters()', () => {
    before(function () {
      if (process.platform === 'darwin') {
        this.skip()
      }
    })

    it('returns an io counters object', () => {
      const ioCounters = process.getIOCounters()
      assert.equal(typeof ioCounters.readOperationCount, 'number')
      assert.equal(typeof ioCounters.writeOperationCount, 'number')
      assert.equal(typeof ioCounters.otherOperationCount, 'number')
      assert.equal(typeof ioCounters.readTransferCount, 'number')
      assert.equal(typeof ioCounters.writeTransferCount, 'number')
      assert.equal(typeof ioCounters.otherTransferCount, 'number')
    })
  })

  describe('process.getHeapStatistics()', () => {
    it('returns heap statistics object', () => {
      const heapStats = process.getHeapStatistics()
      assert.equal(typeof heapStats.totalHeapSize, 'number')
      assert.equal(typeof heapStats.totalHeapSizeExecutable, 'number')
      assert.equal(typeof heapStats.totalPhysicalSize, 'number')
      assert.equal(typeof heapStats.totalAvailableSize, 'number')
      assert.equal(typeof heapStats.usedHeapSize, 'number')
      assert.equal(typeof heapStats.heapSizeLimit, 'number')
      assert.equal(typeof heapStats.mallocedMemory, 'number')
      assert.equal(typeof heapStats.peakMallocedMemory, 'number')
      assert.equal(typeof heapStats.doesZapGarbage, 'boolean')
    })
  })
})
