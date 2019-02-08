const { expect } = require('chai')
const { remote } = require('electron')
const path = require('path')

const { closeWindow } = require('./window-helpers')

const { BrowserWindow } = remote

describe('chrome content scripts', () => {
  const generateTests = (sandboxEnabled, contextIsolationEnabled) => {
    describe(`with sandbox ${sandboxEnabled ? 'enabled' : 'disabled'} and context isolation ${contextIsolationEnabled ? 'enabled' : 'disabled'}`, () => {
      let w

      beforeEach(async () => {
        Object.keys(BrowserWindow.getExtensions()).map(extName => {
          BrowserWindow.removeExtension(extName)
        })
        await closeWindow(w)
        w = new BrowserWindow({
          show: false,
          width: 400,
          height: 400,
          webPreferences: {
            contextIsolation: contextIsolationEnabled,
            sandbox: sandboxEnabled
          }
        })
      })

      afterEach(() => {
        return closeWindow(w).then(() => { w = null })
      })

      const addExtension = (name) => {
        const extensionPath = path.join(__dirname, 'fixtures', 'extensions', name)
        BrowserWindow.addExtension(extensionPath)
      }

      it('should run content script at document_start', (done) => {
        addExtension('content-script-document-start')
        w.webContents.once('dom-ready', () => {
          w.webContents.executeJavaScript('document.documentElement.style.backgroundColor', (result) => {
            expect(result).to.equal('red')
            done()
          })
        })
        w.loadURL('about:blank')
      })

      it('should run content script at document_idle', (done) => {
        addExtension('content-script-document-idle')
        w.loadURL('about:blank')
        w.webContents.executeJavaScript('document.body.style.backgroundColor', (result) => {
          expect(result).to.equal('red')
          done()
        })
      })

      it('should run content script at document_end', (done) => {
        addExtension('content-script-document-end')
        w.webContents.once('did-finish-load', () => {
          w.webContents.executeJavaScript('document.documentElement.style.backgroundColor', (result) => {
            expect(result).to.equal('red')
            done()
          })
        })
        w.loadURL('about:blank')
      })
    })
  }

  generateTests(false, false)
  generateTests(false, true)
  generateTests(true, false)
  generateTests(true, true)
})
