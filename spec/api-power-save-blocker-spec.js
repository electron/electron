const { powerSaveBlocker } = require('electron').remote
const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { expect } = chai
chai.use(dirtyChai)

describe('powerSaveBlocker module', () => {
  it('can be started and stopped', () => {
    expect(powerSaveBlocker.isStarted(-1)).to.be.false()
    const id = powerSaveBlocker.start('prevent-app-suspension')
    expect(id).to.to.be.a('number')
    expect(powerSaveBlocker.isStarted(id)).to.be.true()
    powerSaveBlocker.stop(id)
    expect(powerSaveBlocker.isStarted(id)).to.be.false()
  })
})
