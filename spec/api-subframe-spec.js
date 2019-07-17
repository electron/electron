const { expect } = require('chai')
const { remote } = require('electron')
const path = require('path')

const { emittedNTimes, emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')

const { BrowserWindow, ipcMain } = remote

describe('renderer nodeIntegrationInSubFrames', () => {
  const generateTests = (description, webPreferences) => {
    describe(description, () => {
      const fixtureSuffix = webPreferences.webviewTag ? '-webview' : ''
      let w

      beforeEach(async () => {
        await closeWindow(w)
        w = new BrowserWindow({
          show: false,
          width: 400,
          height: 400,
          webPreferences
        })
      })

      afterEach(() => {
        return closeWindow(w).then(() => {
          w = null
        })
      })

      it('should load preload scripts in top level iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`))
        const [event1, event2] = await detailsPromise
        expect(event1[0].frameId).to.not.equal(event2[0].frameId)
        expect(event1[0].frameId).to.equal(event1[2])
        expect(event2[0].frameId).to.equal(event2[2])
      })

      it('should load preload scripts in nested iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3)
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`))
        const [event1, event2, event3] = await detailsPromise
        expect(event1[0].frameId).to.not.equal(event2[0].frameId)
        expect(event1[0].frameId).to.not.equal(event3[0].frameId)
        expect(event2[0].frameId).to.not.equal(event3[0].frameId)
        expect(event1[0].frameId).to.equal(event1[2])
        expect(event2[0].frameId).to.equal(event2[2])
        expect(event3[0].frameId).to.equal(event3[2])
      })

      it('should correctly reply to the main frame with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`))
        const [event1] = await detailsPromise
        const pongPromise = emittedOnce(ipcMain, 'preload-pong')
        event1[0].reply('preload-ping')
        const details = await pongPromise
        expect(details[1]).to.equal(event1[0].frameId)
      })

      it('should correctly reply to the sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`))
        const [, event2] = await detailsPromise
        const pongPromise = emittedOnce(ipcMain, 'preload-pong')
        event2[0].reply('preload-ping')
        const details = await pongPromise
        expect(details[1]).to.equal(event2[0].frameId)
      })

      it('should correctly reply to the nested sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3)
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`))
        const [, , event3] = await detailsPromise
        const pongPromise = emittedOnce(ipcMain, 'preload-pong')
        event3[0].reply('preload-ping')
        const details = await pongPromise
        expect(details[1]).to.equal(event3[0].frameId)
      })

      it('should not expose globals in main world', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`))
        const details = await detailsPromise
        const senders = details.map(event => event[0].sender)
        await new Promise((resolve) => {
          let resultCount = 0
          senders.forEach(sender => {
            sender.webContents.executeJavaScript('window.isolatedGlobal', result => {
              if (webPreferences.contextIsolation) {
                expect(result).to.be.null()
              } else {
                expect(result).to.equal(true)
              }
              resultCount++
              if (resultCount === senders.length) {
                resolve()
              }
            })
          })
        })
      })
    })
  }

  const generateConfigs = (webPreferences, ...permutations) => {
    const configs = [{ webPreferences, names: [] }]
    for (let i = 0; i < permutations.length; i++) {
      const length = configs.length
      for (let j = 0; j < length; j++) {
        const newConfig = Object.assign({}, configs[j])
        newConfig.webPreferences = Object.assign({},
          newConfig.webPreferences, permutations[i].webPreferences)
        newConfig.names = newConfig.names.slice(0)
        newConfig.names.push(permutations[i].name)
        configs.push(newConfig)
      }
    }

    return configs.map(config => {
      if (config.names.length > 0) {
        config.title = `with ${config.names.join(', ')} on`
      } else {
        config.title = `without anything special turned on`
      }
      delete config.names

      return config
    })
  }

  generateConfigs(
    {
      preload: path.resolve(__dirname, 'fixtures/sub-frames/preload.js'),
      nodeIntegrationInSubFrames: true
    },
    {
      name: 'sandbox',
      webPreferences: { sandbox: true }
    },
    {
      name: 'context isolation',
      webPreferences: { contextIsolation: true }
    },
    {
      name: 'webview',
      webPreferences: { webviewTag: true, preload: false }
    }
  ).forEach(config => {
    generateTests(config.title, config.webPreferences)
  })

  describe('internal <iframe> inside of <webview>', () => {
    let w

    beforeEach(async () => {
      await closeWindow(w)
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          preload: path.resolve(__dirname, 'fixtures/sub-frames/webview-iframe-preload.js'),
          nodeIntegrationInSubFrames: true,
          webviewTag: true
        }
      })
    })

    afterEach(() => {
      return closeWindow(w).then(() => {
        w = null
      })
    })

    it('should not load preload scripts', async () => {
      const promisePass = emittedOnce(ipcMain, 'webview-loaded')
      const promiseFail = emittedOnce(ipcMain, 'preload-in-frame').then(() => {
        throw new Error('preload loaded in internal frame')
      })
      await w.loadURL('about:blank')
      return Promise.race([promisePass, promiseFail])
    })
  })
})
