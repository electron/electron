import { expect } from 'chai'
import * as path from 'path'

import { closeWindow } from './window-helpers'
import { emittedNTimes } from './events-helpers'

import { BrowserWindow, ipcMain, WebContents } from 'electron'

describe('chrome extension content scripts', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  const extensionPath = path.resolve(fixtures, 'extensions')

  const addExtension = (name: string) => BrowserWindow.addExtension(path.resolve(extensionPath, name))
  const removeAllExtensions = () => {
    Object.keys(BrowserWindow.getExtensions()).map(extName => {
      BrowserWindow.removeExtension(extName)
    })
  }

  let responseIdCounter = 0
  const executeJavaScriptInFrame = (webContents: WebContents, frameRoutingId: number, code: string) => {
    return new Promise(resolve => {
      const responseId = responseIdCounter++
      ipcMain.once(`executeJavaScriptInFrame_${responseId}`, (event, result) => {
        resolve(result)
      })
      webContents.send('executeJavaScriptInFrame', frameRoutingId, code, responseId)
    })
  }

  const generateTests = (sandboxEnabled: boolean, contextIsolationEnabled: boolean) => {
    describe(`with sandbox ${sandboxEnabled ? 'enabled' : 'disabled'} and context isolation ${contextIsolationEnabled ? 'enabled' : 'disabled'}`, () => {
      let w: BrowserWindow

      describe('supports "run_at" option', () => {
        beforeEach(async () => {
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
          removeAllExtensions()
          return closeWindow(w).then(() => { w = null as unknown as BrowserWindow })
        })

        it('should run content script at document_start', () => {
          addExtension('content-script-document-start')
          w.webContents.once('dom-ready', async () => {
            const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
            expect(result).to.equal('red')
          })
          w.loadURL('about:blank')
        })

        it('should run content script at document_idle', async () => {
          addExtension('content-script-document-idle')
          w.loadURL('about:blank')
          const result = await w.webContents.executeJavaScript('document.body.style.backgroundColor')
          expect(result).to.equal('red')
        })

        it('should run content script at document_end', () => {
          addExtension('content-script-document-end')
          w.webContents.once('did-finish-load', async () => {
            const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
            expect(result).to.equal('red')
          })
          w.loadURL('about:blank')
        })
      })

      describe('supports "all_frames" option', () => {
        const contentScript = path.resolve(fixtures, 'extensions/content-script')

        // Computed style values
        const COLOR_RED = `rgb(255, 0, 0)`
        const COLOR_BLUE = `rgb(0, 0, 255)`
        const COLOR_TRANSPARENT = `rgba(0, 0, 0, 0)`

        before(() => {
          BrowserWindow.addExtension(contentScript)
        })

        after(() => {
          BrowserWindow.removeExtension('content-script-test')
        })

        beforeEach(() => {
          w = new BrowserWindow({
            show: false,
            webPreferences: {
              // enable content script injection in subframes
              nodeIntegrationInSubFrames: true,
              preload: path.join(contentScript, 'all_frames-preload.js')
            }
          })
        })

        afterEach(() =>
          closeWindow(w).then(() => {
            w = null as unknown as BrowserWindow
          })
        )

        it('applies matching rules in subframes', async () => {
          const detailsPromise = emittedNTimes(w.webContents, 'did-frame-finish-load', 2)
          w.loadFile(path.join(contentScript, 'frame-with-frame.html'))
          const frameEvents = await detailsPromise
          await Promise.all(
            frameEvents.map(async frameEvent => {
              const [, isMainFrame, , frameRoutingId] = frameEvent
              const result: any = await executeJavaScriptInFrame(
                w.webContents,
                frameRoutingId,
                `(() => {
                  const a = document.getElementById('all_frames_enabled')
                  const b = document.getElementById('all_frames_disabled')
                  return {
                    enabledColor: getComputedStyle(a).backgroundColor,
                    disabledColor: getComputedStyle(b).backgroundColor
                  }
                })()`
              )
              expect(result.enabledColor).to.equal(COLOR_RED)
              if (isMainFrame) {
                expect(result.disabledColor).to.equal(COLOR_BLUE)
              } else {
                expect(result.disabledColor).to.equal(COLOR_TRANSPARENT) // null color
              }
            })
          )
        })
      })
    })
  }

  generateTests(false, false)
  generateTests(false, true)
  generateTests(true, false)
  generateTests(true, true)
})
