const { globalShortcut } = require('electron').remote

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const isCI = require('electron').remote.getGlobal('isCi')

const { expect } = chai
chai.use(dirtyChai)

describe('globalShortcut module', () => {
  before(function () {
    if (isCI && process.platform === 'win32') {
      this.skip()
    }
  })

  beforeEach(() => {
    globalShortcut.unregisterAll()
  })

  it('can register and unregister single accelerators', () => {
    const accelerator = 'CmdOrCtrl+A+B+C'

    expect(globalShortcut.isRegistered(accelerator)).to.be.false()
    globalShortcut.register(accelerator, () => {})
    expect(globalShortcut.isRegistered(accelerator)).to.be.true()
    globalShortcut.unregister(accelerator)
    expect(globalShortcut.isRegistered(accelerator)).to.be.false()

    expect(globalShortcut.isRegistered(accelerator)).to.be.false()
    globalShortcut.register(accelerator, () => {})
    expect(globalShortcut.isRegistered(accelerator)).to.be.true()
    globalShortcut.unregisterAll()
    expect(globalShortcut.isRegistered(accelerator)).to.be.false()
  })

  it('can register and unregister multiple accelerators', () => {
    const accelerators = ['CmdOrCtrl+X', 'CmdOrCtrl+Y']

    expect(globalShortcut.isRegistered(accelerators[0])).to.be.false()
    expect(globalShortcut.isRegistered(accelerators[1])).to.be.false()

    globalShortcut.registerAll(accelerators, () => {})

    expect(globalShortcut.isRegistered(accelerators[0])).to.be.true()
    expect(globalShortcut.isRegistered(accelerators[1])).to.be.true()

    globalShortcut.unregisterAll()

    expect(globalShortcut.isRegistered(accelerators[0])).to.be.false()
    expect(globalShortcut.isRegistered(accelerators[1])).to.be.false()
  })
})
