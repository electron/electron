const assert = require('assert')

describe('process module', function () {
  describe('process.getCPUUsage()', function () {
    it('returns a cpu usage object', function () {
      var cpuUsage = process.getCPUUsage()
      assert.equal(typeof cpuUsage.percentCPUUsage, 'number')
      assert.equal(typeof cpuUsage.idleWakeupsPerSecond, 'number')
    })
  })

  describe('process.getIOCounters()', function () {
    it('returns an io counters object', function () {
      var ioCounters = process.getIOCounters()
      assert.ok(ioCounters.readOperationCount > 0, 'read operation count not > 0')
      assert.ok(ioCounters.writeOperationCount > 0, 'write operation count not > 0')
      assert.ok(ioCounters.otherOperationCount > 0, 'other operation count not > 0')
      assert.ok(ioCounters.readTransferCount > 0, 'read transfer count not > 0')
      assert.ok(ioCounters.writeTransferCount > 0, 'write transfer count not > 0')
      assert.ok(ioCounters.otherTransferCount > 0, 'other transfer count not > 0')
    })
  })
})
