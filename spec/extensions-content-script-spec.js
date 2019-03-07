const path = require('path')

const { expect } = require('chai')
const { remote } = require('electron')

const { closeWindow } = require('./window-helpers')
const { emittedNTimes } = require('./events-helpers')

const { BrowserWindow, ipcMain } = remote

describe('chrome extension content scripts', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')

  describe('supports "all_frames" option', () => {
    const contentScript = path.resolve(fixtures, 'extensions/content-script')

    // Computed style values
    const COLOR_RED = `rgb(255, 0, 0)`
    const COLOR_BLUE = `rgb(0, 0, 255)`
    const COLOR_TRANSPARENT = `rgba(0, 0, 0, 0)`

    let responseIdCounter = 0
    const executeJavaScriptInFrame = (webContents, frameRoutingId, code) => {
      return new Promise(resolve => {
        const responseId = responseIdCounter++
        ipcMain.once(`executeJavaScriptInFrame_${responseId}`, (event, result) => {
          resolve(result)
        })
        webContents.send('executeJavaScriptInFrame', frameRoutingId, code, responseId)
      })
    }

    before(() => {
      BrowserWindow.addExtension(contentScript)
    })

    after(() => {
      BrowserWindow.removeExtension('content-script-test')
    })

    let w

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
        w = null
      })
    )

    it('applies matching rules in subframes', async () => {
      const detailsPromise = emittedNTimes(w.webContents, 'did-frame-finish-load', 2)
      w.loadFile(path.join(contentScript, 'frame-with-frame.html'))
      const frameEvents = await detailsPromise
      await Promise.all(
        frameEvents.map(async frameEvent => {
          const [, isMainFrame, , frameRoutingId] = frameEvent
          const result = await executeJavaScriptInFrame(
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
