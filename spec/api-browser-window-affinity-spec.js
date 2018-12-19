'use strict'

const { expect } = require('chai')
const path = require('path')

const { remote } = require('electron')
const { ipcMain, BrowserWindow } = remote
const { closeWindow } = require('./window-helpers')

describe('BrowserWindow with affinity module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  const myAffinityName = 'myAffinity'
  const myAffinityNameUpper = 'MYAFFINITY'
  const anotherAffinityName = 'anotherAffinity'

  async function createWindowWithWebPrefs (webPrefs) {
    const w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: webPrefs || {}
    })
    await w.loadFile(path.join(fixtures, 'api', 'blank.html'))
    return w
  }

  function testAffinityProcessIds (name, webPreferences = {}) {
    describe(name, () => {
      let mAffinityWindow
      before(async () => {
        mAffinityWindow = await createWindowWithWebPrefs({ affinity: myAffinityName, ...webPreferences })
      })

      after(async () => {
        await closeWindow(mAffinityWindow, { assertSingleWindow: false })
        mAffinityWindow = null
      })

      it('should have a different process id than a default window', async () => {
        const w = await createWindowWithWebPrefs({ ...webPreferences })
        const affinityID = mAffinityWindow.webContents.getOSProcessId()
        const wcID = w.webContents.getOSProcessId()

        expect(affinityID).to.not.equal(wcID, 'Should have different OS process IDs')
        await closeWindow(w, { assertSingleWindow: false })
      })

      it(`should have a different process id than a window with a different affinity '${anotherAffinityName}'`, async () => {
        const w = await createWindowWithWebPrefs({ affinity: anotherAffinityName, ...webPreferences })
        const affinityID = mAffinityWindow.webContents.getOSProcessId()
        const wcID = w.webContents.getOSProcessId()

        expect(affinityID).to.not.equal(wcID, 'Should have different OS process IDs')
        await closeWindow(w, { assertSingleWindow: false })
      })

      it(`should have the same OS process id than a window with the same affinity '${myAffinityName}'`, async () => {
        const w = await createWindowWithWebPrefs({ affinity: myAffinityName, ...webPreferences })
        const affinityID = mAffinityWindow.webContents.getOSProcessId()
        const wcID = w.webContents.getOSProcessId()

        expect(affinityID).to.equal(wcID, 'Should have the same OS process ID')
        await closeWindow(w, { assertSingleWindow: false })
      })

      it(`should have the same OS process id than a window with an equivalent affinity '${myAffinityNameUpper}' (case insensitive)`, async () => {
        const w = await createWindowWithWebPrefs({ affinity: myAffinityNameUpper, ...webPreferences })
        const affinityID = mAffinityWindow.webContents.getOSProcessId()
        const wcID = w.webContents.getOSProcessId()

        expect(affinityID).to.equal(wcID, 'Should have the same OS process ID')
        await closeWindow(w, { assertSingleWindow: false })
      })
    })
  }

  testAffinityProcessIds(`BrowserWindow with an affinity '${myAffinityName}'`)
  testAffinityProcessIds(`BrowserWindow with an affinity '${myAffinityName}' and sandbox enabled`, { sandbox: true })
  testAffinityProcessIds(`BrowserWindow with an affinity '${myAffinityName}' and nativeWindowOpen enabled`, { nativeWindowOpen: true })

  describe(`BrowserWindow with an affinity : nodeIntegration=false`, () => {
    const preload = path.join(fixtures, 'module', 'send-later.js')
    const affinityWithNodeTrue = 'affinityWithNodeTrue'
    const affinityWithNodeFalse = 'affinityWithNodeFalse'

    function testNodeIntegration (present) {
      return new Promise((resolve, reject) => {
        ipcMain.once('answer', (event, typeofProcess, typeofBuffer) => {
          if (present) {
            expect(typeofProcess).to.not.equal('undefined')
            expect(typeofBuffer).to.not.equal('undefined')
          } else {
            expect(typeofProcess).to.equal('undefined')
            expect(typeofBuffer).to.equal('undefined')
          }
          resolve()
        })
      })
    }

    it('disables node integration when specified to false', async () => {
      const [, w] = await Promise.all([
        testNodeIntegration(false),
        createWindowWithWebPrefs({
          affinity: affinityWithNodeTrue,
          preload,
          nodeIntegration: false
        })
      ])
      await closeWindow(w, { assertSingleWindow: false })
    })
    it('disables node integration when first window is false', async () => {
      const [, w1] = await Promise.all([
        testNodeIntegration(false),
        createWindowWithWebPrefs({
          affinity: affinityWithNodeTrue,
          preload,
          nodeIntegration: false
        })
      ])
      const [, w2] = await Promise.all([
        testNodeIntegration(false),
        createWindowWithWebPrefs({
          affinity: affinityWithNodeTrue,
          preload,
          nodeIntegration: true
        })
      ])
      await Promise.all([
        closeWindow(w1, { assertSingleWindow: false }),
        closeWindow(w2, { assertSingleWindow: false })
      ])
    })

    it('enables node integration when specified to true', async () => {
      const [, w] = await Promise.all([
        testNodeIntegration(true),
        createWindowWithWebPrefs({
          affinity: affinityWithNodeFalse,
          preload,
          nodeIntegration: true
        })
      ])
      await closeWindow(w, { assertSingleWindow: false })
    })

    it('enables node integration when first window is true', async () => {
      const [, w1] = await Promise.all([
        testNodeIntegration(true),
        createWindowWithWebPrefs({
          affinity: affinityWithNodeFalse,
          preload,
          nodeIntegration: true
        })
      ])
      const [, w2] = await Promise.all([
        testNodeIntegration(true),
        createWindowWithWebPrefs({
          affinity: affinityWithNodeFalse,
          preload,
          nodeIntegration: false
        })
      ])
      await Promise.all([
        closeWindow(w1, { assertSingleWindow: false }),
        closeWindow(w2, { assertSingleWindow: false })
      ])
    })
  })
})
