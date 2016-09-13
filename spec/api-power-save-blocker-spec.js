const {powerSaveBlocker} = require('electron').remote
const assert = require('assert')

describe('powerSaveBlocker module', () => {
  it('can be started and stopped', () => {
    assert.equal(powerSaveBlocker.isStarted(-1), false)
    const id = powerSaveBlocker.start('prevent-app-suspension')
    assert.ok(id != null)
    assert.equal(powerSaveBlocker.isStarted(id), true)
    powerSaveBlocker.stop(id)
    assert.equal(powerSaveBlocker.isStarted(id), false)
  })
})
