const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const { closeWindow } = require('./window-helpers')
const { remote, webFrame } = require('electron')
const { BrowserWindow, ipcMain } = remote

const { expect } = chai
chai.use(dirtyChai)

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('webFrame module', function () {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w = null

  afterEach(function () {
    return closeWindow(w).then(function () { w = null })
  })

  it('supports setting the visual and layout zoom level limits', function () {
    assert.doesNotThrow(function () {
      webFrame.setVisualZoomLevelLimits(1, 50)
      webFrame.setLayoutZoomLevelLimits(0, 25)
    })
  })

  it('calls a spellcheck provider', async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true
      }
    })
    await w.loadFile(path.join(fixtures, 'pages', 'webframe-spell-check.html'))
    w.focus()
    await w.webContents.executeJavaScript('document.querySelector("input").focus()', true)

    const spellCheckerFeedback =
      new Promise(resolve => {
        ipcMain.on('spec-spell-check', (e, words, callback) => {
          if (words.length === 2) {
            // The promise is resolved only after this event is received twice
            // Array contains only 1 word first time and 2 the next time
            resolve([words, callback])
          }
        })
      })
    const inputText = 'spleling test '
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'char', keyCode })
    }
    const [words, callback] = await spellCheckerFeedback
    expect(words).to.deep.equal(['spleling', 'test'])
    expect(callback).to.be.true()
  })

  it('top is self for top frame', () => {
    expect(webFrame.top.context).to.equal(webFrame.context)
  })

  it('opener is null for top frame', () => {
    expect(webFrame.opener).to.be.null()
  })

  it('firstChild is null for top frame', () => {
    expect(webFrame.firstChild).to.be.null()
  })

  it('getFrameForSelector() does not crash when not found', () => {
    expect(webFrame.getFrameForSelector('unexist-selector')).to.be.null()
  })

  it('findFrameByName() does not crash when not found', () => {
    expect(webFrame.findFrameByName('unexist-name')).to.be.null()
  })

  it('findFrameByRoutingId() does not crash when not found', () => {
    expect(webFrame.findFrameByRoutingId(-1)).to.be.null()
  })
})
