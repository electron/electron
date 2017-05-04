const assert = require('assert')

describe('process module', function () {
  describe('process.getCPUUsage()', function () {
    it('returns a cpu usage object', function () {
      const cpuUsage = process.getCPUUsage()
      assert.equal(typeof cpuUsage.percentCPUUsage, 'number')
      assert.equal(typeof cpuUsage.idleWakeupsPerSecond, 'number')
    })
  })

  describe('process.getIOCounters()', function () {
    it('returns an io counters object', function () {
      if (process.platform === 'darwin') {
        return
      }
      const ioCounters = process.getIOCounters()
      assert.equal(typeof ioCounters.readOperationCount, 'number')
      assert.equal(typeof ioCounters.writeOperationCount, 'number')
      assert.equal(typeof ioCounters.otherOperationCount, 'number')
      assert.equal(typeof ioCounters.readTransferCount, 'number')
      assert.equal(typeof ioCounters.writeTransferCount, 'number')
      assert.equal(typeof ioCounters.otherTransferCount, 'number')
    })
  })
})
