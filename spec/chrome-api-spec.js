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

    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'getManifest' }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, manifestString] = await promise
    const manifest = JSON.parse(manifestString)

    expect(manifest.name).to.equal(actualManifest.name)
    expect(manifest.content_scripts.length).to.equal(actualManifest.content_scripts.length)
  })

  it('chrome.tabs.sendMessage receives the response', async function () {
    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'sendMessage', args: ['Hello World!'] }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, responseString] = await promise
    const response = JSON.parse(responseString)

    expect(response.message).to.equal('Hello World!')
    expect(response.tabId).to.equal(w.webContents.id)
  })

  it('chrome.tabs.executeScript receives the response', async function () {
    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'executeScript', args: ['1 + 2'] }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, responseString] = await promise
    const response = JSON.parse(responseString)

    expect(response).to.equal(3)
  })
})
