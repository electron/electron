const fs = require('fs')
const path = require('path')

const { expect } = require('chai')
const { remote } = require('electron')

const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')

const { BrowserWindow } = remote

describe('chrome api', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w

  before(() => {
    BrowserWindow.addExtension(path.join(fixtures, 'extensions/chrome-api'))
  })

  after(() => {
    BrowserWindow.removeExtension('chrome-api')
  })

  beforeEach(() => {
    w = new BrowserWindow({
      show: false
    })
  })

  afterEach(() => closeWindow(w).then(() => { w = null }))

  it('runtime.getManifest returns extension manifest', async () => {
    const actualManifest = (() => {
      const data = fs.readFileSync(path.join(fixtures, 'extensions/chrome-api/manifest.json'), 'utf-8')
      return JSON.parse(data)
    })()

    w.loadURL('about:blank')

    const p = emittedOnce(w.webContents, 'console-message')
    w.webContents.executeJavaScript(`window.postMessage('getManifest', '*')`)
    const [,, manifestString] = await p
    const manifest = JSON.parse(manifestString)

    expect(manifest.name).to.equal(actualManifest.name)
    expect(manifest.content_scripts.length).to.equal(actualManifest.content_scripts.length)
  })
})
