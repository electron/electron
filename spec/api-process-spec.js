const { remote } = require('electron')
const fs = require('fs')
const path = require('path')

const { expect } = require('chai')

describe('process module', () => {
  describe('process.getCreationTime()', () => {
    it('returns a creation time', () => {
      const creationTime = process.getCreationTime()
      expect(creationTime).to.be.a('number').and.be.at.least(0)
    })
  })

  describe('process.getCPUUsage()', () => {
    it('returns a cpu usage object', () => {
      const cpuUsage = process.getCPUUsage()
      expect(cpuUsage.percentCPUUsage).to.be.a('number')
      expect(cpuUsage.idleWakeupsPerSecond).to.be.a('number')
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
      expect(ioCounters.readOperationCount).to.be.a('number')
      expect(ioCounters.writeOperationCount).to.be.a('number')
      expect(ioCounters.otherOperationCount).to.be.a('number')
      expect(ioCounters.readTransferCount).to.be.a('number')
      expect(ioCounters.writeTransferCount).to.be.a('number')
      expect(ioCounters.otherTransferCount).to.be.a('number')
    })
  })

  // FIXME: Chromium 67 - getProcessMemoryInfo has been removed
  // describe('process.getProcessMemoryInfo()', () => {
  //   it('returns process memory info object', () => {
  //     const processMemoryInfo = process.getProcessMemoryInfo()
  //     expect(processMemoryInfo.peakWorkingSetSize).to.be.a('number')
  //     expect(processMemoryInfo.privateBytes).to.be.a('number')
  //     expect(processMemoryInfo.sharedBytes).to.be.a('number')
  //     expect(processMemoryInfo.workingSetSize).to.be.a('number')
  //   })
  // })

  describe('process.getSystemMemoryInfo()', () => {
    it('returns system memory info object', () => {
      const systemMemoryInfo = process.getSystemMemoryInfo()
      expect(systemMemoryInfo.free).to.be.a('number')
      expect(systemMemoryInfo.total).to.be.a('number')
    })
  })

  describe('process.getHeapStatistics()', () => {
    it('returns heap statistics object', () => {
      const heapStats = process.getHeapStatistics()
      expect(heapStats.totalHeapSize).to.be.a('number')
      expect(heapStats.totalHeapSizeExecutable).to.be.a('number')
      expect(heapStats.totalPhysicalSize).to.be.a('number')
      expect(heapStats.totalAvailableSize).to.be.a('number')
      expect(heapStats.usedHeapSize).to.be.a('number')
      expect(heapStats.heapSizeLimit).to.be.a('number')
      expect(heapStats.mallocedMemory).to.be.a('number')
      expect(heapStats.peakMallocedMemory).to.be.a('number')
      expect(heapStats.doesZapGarbage).to.be.a('boolean')
    })
  })

  describe('process.takeHeapSnapshot()', () => {
    it('returns true on success', () => {
      const filePath = path.join(remote.app.getPath('temp'), 'test.heapsnapshot')

      const cleanup = () => {
        try {
          fs.unlinkSync(filePath)
        } catch (e) {
          // ignore error
        }
      }

      try {
        const success = process.takeHeapSnapshot(filePath)
        expect(success).to.be.true()
        const stats = fs.statSync(filePath)
        expect(stats.size).not.to.be.equal(0)
      } finally {
        cleanup()
      }
    })

    it('returns false on failure', () => {
      const success = process.takeHeapSnapshot('')
      expect(success).to.be.false()
    })
  })
})
