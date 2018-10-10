'use strict'

const assert = require('assert')
const http = require('http')
const path = require('path')
const {closeWindow} = require('./window-helpers')
const {emittedOnce} = require('./events-helpers')

const {ipcRenderer, remote, clipboard} = require('electron')
const {BrowserWindow, webContents, ipcMain, session} = remote

const isCi = remote.getGlobal('isCi')

/* The whole webContents API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('webContents module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false
      }
    })
  })

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('getAllWebContents() API', () => {
    it('returns an array of web contents', (done) => {
      w.webContents.on('devtools-opened', () => {
        const all = webContents.getAllWebContents().sort((a, b) => {
          return a.getId() - b.getId()
        })

        assert.ok(all.length >= 4)
        assert.equal(all[0].getType(), 'window')
        assert.equal(all[all.length - 2].getType(), 'remote')
        assert.equal(all[all.length - 1].getType(), 'webview')

        done()
      })

      w.loadURL(`file://${path.join(fixtures, 'pages', 'webview-zoom-factor.html')}`)
      w.webContents.openDevTools()
    })
  })

  describe('getFocusedWebContents() API', () => {
    it('returns the focused web contents', (done) => {
      if (isCi) return done()

      const specWebContents = remote.getCurrentWebContents()
      assert.equal(specWebContents.getId(), webContents.getFocusedWebContents().getId())

      specWebContents.once('devtools-opened', () => {
        assert.equal(specWebContents.devToolsWebContents.getId(), webContents.getFocusedWebContents().getId())
        specWebContents.closeDevTools()
      })

      specWebContents.once('devtools-closed', () => {
        assert.equal(specWebContents.getId(), webContents.getFocusedWebContents().getId())
        done()
      })

      specWebContents.openDevTools()
    })

    it('does not crash when called on a detached dev tools window', (done) => {
      const specWebContents = w.webContents

      specWebContents.once('devtools-opened', () => {
        assert.doesNotThrow(() => {
          webContents.getFocusedWebContents()
        })
        specWebContents.closeDevTools()
      })

      specWebContents.once('devtools-closed', () => {
        assert.doesNotThrow(() => {
          webContents.getFocusedWebContents()
        })
        done()
      })

      specWebContents.openDevTools({mode: 'detach'})
      w.inspectElement(100, 100)
    })
  })

  describe('setDevToolsWebContents() API', () => {
    it('sets arbitry webContents as devtools', (done) => {
      let devtools = new BrowserWindow({show: false})
      devtools.webContents.once('dom-ready', () => {
        assert.ok(devtools.getURL().startsWith('chrome-devtools://devtools'))
        devtools.webContents.executeJavaScript('InspectorFrontendHost.constructor.name', (name) => {
          assert.ok(name, 'InspectorFrontendHostImpl')
          devtools.destroy()
          done()
        })
      })
      w.webContents.setDevToolsWebContents(devtools.webContents)
      w.webContents.openDevTools()
    })
  })

  describe('isFocused() API', () => {
    it('returns false when the window is hidden', () => {
      BrowserWindow.getAllWindows().forEach((window) => {
        assert.equal(!window.isVisible() && window.webContents.isFocused(), false)
      })
    })
  })

  describe('getWebPreferences() API', () => {
    it('should not crash when called for devTools webContents', (done) => {
      w.webContents.openDevTools()
      w.webContents.once('devtools-opened', () => {
        assert(!w.devToolsWebContents.getWebPreferences())
        done()
      })
    })
  })

  describe('before-input-event event', () => {
    it('can prevent document keyboard events', (done) => {
      w.loadURL(`file://${path.join(__dirname, 'fixtures', 'pages', 'key-events.html')}`)
      w.webContents.once('did-finish-load', () => {
        ipcMain.once('keydown', (event, key) => {
          assert.equal(key, 'b')
          done()
        })

        ipcRenderer.send('prevent-next-input-event', 'a', w.webContents.id)
        w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'a'})
        w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'b'})
      })
    })

    it('has the correct properties', (done) => {
      w.loadURL(`file://${path.join(__dirname, 'fixtures', 'pages', 'base-page.html')}`)
      w.webContents.once('did-finish-load', () => {
        const testBeforeInput = (opts) => {
          return new Promise((resolve, reject) => {
            w.webContents.once('before-input-event', (event, input) => {
              assert.equal(input.type, opts.type)
              assert.equal(input.key, opts.key)
              assert.equal(input.code, opts.code)
              assert.equal(input.isAutoRepeat, opts.isAutoRepeat)
              assert.equal(input.shift, opts.shift)
              assert.equal(input.control, opts.control)
              assert.equal(input.alt, opts.alt)
              assert.equal(input.meta, opts.meta)
              resolve()
            })

            const modifiers = []
            if (opts.shift) modifiers.push('shift')
            if (opts.control) modifiers.push('control')
            if (opts.alt) modifiers.push('alt')
            if (opts.meta) modifiers.push('meta')
            if (opts.isAutoRepeat) modifiers.push('isAutoRepeat')

            w.webContents.sendInputEvent({
              type: opts.type,
              keyCode: opts.keyCode,
              modifiers: modifiers
            })
          })
        }

        Promise.resolve().then(() => {
          return testBeforeInput({
            type: 'keyDown',
            key: 'A',
            code: 'KeyA',
            keyCode: 'a',
            shift: true,
            control: true,
            alt: true,
            meta: true,
            isAutoRepeat: true
          })
        }).then(() => {
          return testBeforeInput({
            type: 'keyUp',
            key: '.',
            code: 'Period',
            keyCode: '.',
            shift: false,
            control: true,
            alt: true,
            meta: false,
            isAutoRepeat: false
          })
        }).then(() => {
          return testBeforeInput({
            type: 'keyUp',
            key: '!',
            code: 'Digit1',
            keyCode: '1',
            shift: true,
            control: false,
            alt: false,
            meta: true,
            isAutoRepeat: false
          })
        }).then(() => {
          return testBeforeInput({
            type: 'keyUp',
            key: 'Tab',
            code: 'Tab',
            keyCode: 'Tab',
            shift: false,
            control: true,
            alt: false,
            meta: false,
            isAutoRepeat: true
          })
        }).then(done).catch(done)
      })
    })
  })

  describe('devtools window', () => {
    let testFn = it
    if (process.platform === 'darwin' && isCi) {
      testFn = it.skip
    }
    try {
      // We have other tests that check if native modules work, if we fail to require
      // robotjs let's skip this test to avoid false negatives
      require('robotjs')
    } catch (err) {
      testFn = it.skip
    }
    testFn('can receive and handle menu events', async function () {
      this.timeout(5000)
      w.show()
      w.loadFile(path.join(fixtures, 'pages', 'key-events.html'))
      // Ensure the devtools are loaded
      w.webContents.closeDevTools()
      const opened = emittedOnce(w.webContents, 'devtools-opened')
      w.webContents.openDevTools()
      await opened
      await emittedOnce(w.webContents.devToolsWebContents, 'did-finish-load')
      w.webContents.devToolsWebContents.focus()

      // Focus an input field
      await w.webContents.devToolsWebContents.executeJavaScript(
        `const input = document.createElement('input');
        document.body.innerHTML = '';
        document.body.appendChild(input)
        input.focus();`
      )

      // Write something to the clipboard
      clipboard.writeText('test value')

      // Fake a paste request using robotjs to emulate a REAL keyboard paste event
      require('robotjs').keyTap('v', process.platform === 'darwin' ? ['command'] : ['control'])

      const start = Date.now()
      let val

      // Check every now and again for the pasted value (paste is async)
      while (val !== 'test value' && Date.now() - start <= 1000) {
        val = await w.webContents.devToolsWebContents.executeJavaScript(
          `document.querySelector('input').value`
        )
        await new Promise(resolve => setTimeout(resolve, 10))
      }

      // Once we're done expect the paste to have been successful
      assert.equal(val, 'test value', 'value should eventually become the pasted value')
    })
  })

  describe('sendInputEvent(event)', () => {
    beforeEach((done) => {
      w.loadURL(`file://${path.join(__dirname, 'fixtures', 'pages', 'key-events.html')}`)
      w.webContents.once('did-finish-load', () => done())
    })

    it('can send keydown events', (done) => {
      ipcMain.once('keydown', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        assert.equal(key, 'a')
        assert.equal(code, 'KeyA')
        assert.equal(keyCode, 65)
        assert.equal(shiftKey, false)
        assert.equal(ctrlKey, false)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'A'})
    })

    it('can send keydown events with modifiers', (done) => {
      ipcMain.once('keydown', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        assert.equal(key, 'Z')
        assert.equal(code, 'KeyZ')
        assert.equal(keyCode, 90)
        assert.equal(shiftKey, true)
        assert.equal(ctrlKey, true)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'Z', modifiers: ['shift', 'ctrl']})
    })

    it('can send keydown events with special keys', (done) => {
      ipcMain.once('keydown', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        assert.equal(key, 'Tab')
        assert.equal(code, 'Tab')
        assert.equal(keyCode, 9)
        assert.equal(shiftKey, false)
        assert.equal(ctrlKey, false)
        assert.equal(altKey, true)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'Tab', modifiers: ['alt']})
    })

    it('can send char events', (done) => {
      ipcMain.once('keypress', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        assert.equal(key, 'a')
        assert.equal(code, 'KeyA')
        assert.equal(keyCode, 65)
        assert.equal(shiftKey, false)
        assert.equal(ctrlKey, false)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'A'})
      w.webContents.sendInputEvent({type: 'char', keyCode: 'A'})
    })

    it('can send char events with modifiers', (done) => {
      ipcMain.once('keypress', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        assert.equal(key, 'Z')
        assert.equal(code, 'KeyZ')
        assert.equal(keyCode, 90)
        assert.equal(shiftKey, true)
        assert.equal(ctrlKey, true)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'Z'})
      w.webContents.sendInputEvent({type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl']})
    })
  })

  it('supports inserting CSS', (done) => {
    w.loadURL('about:blank')
    w.webContents.insertCSS('body { background-repeat: round; }')
    w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")', (result) => {
      assert.equal(result, 'round')
      done()
    })
  })

  it('supports inspecting an element in the devtools', (done) => {
    w.loadURL('about:blank')
    w.webContents.once('devtools-opened', () => {
      done()
    })
    w.webContents.inspectElement(10, 10)
  })

  describe('startDrag({file, icon})', () => {
    it('throws errors for a missing file or a missing/empty icon', () => {
      assert.throws(() => {
        w.webContents.startDrag({icon: path.join(__dirname, 'fixtures', 'assets', 'logo.png')})
      }, /Must specify either 'file' or 'files' option/)

      assert.throws(() => {
        w.webContents.startDrag({file: __filename})
      }, /Must specify 'icon' option/)

      if (process.platform === 'darwin') {
        assert.throws(() => {
          w.webContents.startDrag({file: __filename, icon: __filename})
        }, /Must specify non-empty 'icon' option/)
      }
    })
  })

  describe('focus()', () => {
    describe('when the web contents is hidden', () => {
      it('does not blur the focused window', (done) => {
        ipcMain.once('answer', (event, parentFocused, childFocused) => {
          assert.equal(parentFocused, true)
          assert.equal(childFocused, false)
          done()
        })
        w.show()
        w.loadURL(`file://${path.join(__dirname, 'fixtures', 'pages', 'focus-web-contents.html')}`)
      })
    })
  })

  describe('getOSProcessId()', () => {
    it('returns a valid procress id', (done) => {
      assert.strictEqual(w.webContents.getOSProcessId(), 0)

      w.webContents.once('did-finish-load', () => {
        const pid = w.webContents.getOSProcessId()
        assert.equal(typeof pid, 'number')
        assert(pid > 0, `pid ${pid} is not greater than 0`)
        done()
      })
      w.loadURL('about:blank')
    })
  })

  describe('zoom api', () => {
    const zoomScheme = remote.getGlobal('zoomScheme')
    const hostZoomMap = {
      host1: 0.3,
      host2: 0.7,
      host3: 0.2
    }

    before((done) => {
      const protocol = session.defaultSession.protocol
      protocol.registerStringProtocol(zoomScheme, (request, callback) => {
        const response = `<script>
                            const {ipcRenderer, remote} = require('electron')
                            ipcRenderer.send('set-zoom', window.location.hostname)
                            ipcRenderer.on(window.location.hostname + '-zoom-set', () => {
                              remote.getCurrentWebContents().getZoomLevel((zoomLevel) => {
                                ipcRenderer.send(window.location.hostname + '-zoom-level', zoomLevel)
                              })
                            })
                          </script>`
        callback({data: response, mimeType: 'text/html'})
      }, (error) => done(error))
    })

    after((done) => {
      const protocol = session.defaultSession.protocol
      protocol.unregisterProtocol(zoomScheme, (error) => done(error))
    })

    it('can set the correct zoom level', (done) => {
      w.loadURL('about:blank')
      w.webContents.on('did-finish-load', () => {
        w.webContents.getZoomLevel((zoomLevel) => {
          assert.equal(zoomLevel, 0.0)
          w.webContents.setZoomLevel(0.5)
          w.webContents.getZoomLevel((zoomLevel) => {
            assert.equal(zoomLevel, 0.5)
            w.webContents.setZoomLevel(0)
            done()
          })
        })
      })
    })

    it('can persist zoom level across navigation', (done) => {
      let finalNavigation = false
      ipcMain.on('set-zoom', (e, host) => {
        const zoomLevel = hostZoomMap[host]
        if (!finalNavigation) w.webContents.setZoomLevel(zoomLevel)
        e.sender.send(`${host}-zoom-set`)
      })
      ipcMain.on('host1-zoom-level', (e, zoomLevel) => {
        const expectedZoomLevel = hostZoomMap.host1
        assert.equal(zoomLevel, expectedZoomLevel)
        if (finalNavigation) {
          done()
        } else {
          w.loadURL(`${zoomScheme}://host2`)
        }
      })
      ipcMain.once('host2-zoom-level', (e, zoomLevel) => {
        const expectedZoomLevel = hostZoomMap.host2
        assert.equal(zoomLevel, expectedZoomLevel)
        finalNavigation = true
        w.webContents.goBack()
      })
      w.loadURL(`${zoomScheme}://host1`)
    })

    it('can propagate zoom level across same session', (done) => {
      const w2 = new BrowserWindow({
        show: false
      })
      w2.webContents.on('did-finish-load', () => {
        w.webContents.getZoomLevel((zoomLevel1) => {
          assert.equal(zoomLevel1, hostZoomMap.host3)
          w2.webContents.getZoomLevel((zoomLevel2) => {
            assert.equal(zoomLevel1, zoomLevel2)
            w2.setClosable(true)
            w2.close()
            done()
          })
        })
      })
      w.webContents.on('did-finish-load', () => {
        w.webContents.setZoomLevel(hostZoomMap.host3)
        w2.loadURL(`${zoomScheme}://host3`)
      })
      w.loadURL(`${zoomScheme}://host3`)
    })

    it('cannot propagate zoom level across different session', (done) => {
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'temp'
        }
      })
      const protocol = w2.webContents.session.protocol
      protocol.registerStringProtocol(zoomScheme, (request, callback) => {
        callback('hello')
      }, (error) => {
        if (error) return done(error)
        w2.webContents.on('did-finish-load', () => {
          w.webContents.getZoomLevel((zoomLevel1) => {
            assert.equal(zoomLevel1, hostZoomMap.host3)
            w2.webContents.getZoomLevel((zoomLevel2) => {
              assert.equal(zoomLevel2, 0)
              assert.notEqual(zoomLevel1, zoomLevel2)
              protocol.unregisterProtocol(zoomScheme, (error) => {
                if (error) return done(error)
                w2.setClosable(true)
                w2.close()
                done()
              })
            })
          })
        })
        w.webContents.on('did-finish-load', () => {
          w.webContents.setZoomLevel(hostZoomMap.host3)
          w2.loadURL(`${zoomScheme}://host3`)
        })
        w.loadURL(`${zoomScheme}://host3`)
      })
    })

    it('can persist when it contains iframe', (done) => {
      const server = http.createServer((req, res) => {
        setTimeout(() => {
          res.end()
        }, 200)
      })
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + server.address().port
        const content = `<iframe src=${url}></iframe>`
        w.webContents.on('did-frame-finish-load', (e, isMainFrame) => {
          if (!isMainFrame) {
            w.webContents.getZoomLevel((zoomLevel) => {
              assert.equal(zoomLevel, 2.0)
              w.webContents.setZoomLevel(0)
              server.close()
              done()
            })
          }
        })
        w.webContents.on('dom-ready', () => {
          w.webContents.setZoomLevel(2.0)
        })
        w.loadURL(`data:text/html,${content}`)
      })
    })

    it('cannot propagate when used with webframe', (done) => {
      let finalZoomLevel = 0
      const w2 = new BrowserWindow({
        show: false
      })
      w2.webContents.on('did-finish-load', () => {
        w.webContents.getZoomLevel((zoomLevel1) => {
          assert.equal(zoomLevel1, finalZoomLevel)
          w2.webContents.getZoomLevel((zoomLevel2) => {
            assert.equal(zoomLevel2, 0)
            assert.notEqual(zoomLevel1, zoomLevel2)
            w2.setClosable(true)
            w2.close()
            done()
          })
        })
      })
      ipcMain.once('temporary-zoom-set', (e, zoomLevel) => {
        w2.loadURL(`file://${fixtures}/pages/c.html`)
        finalZoomLevel = zoomLevel
      })
      w.loadURL(`file://${fixtures}/pages/webframe-zoom.html`)
    })

    it('cannot persist zoom level after navigation with webFrame', (done) => {
      let initialNavigation = true
      const source = `
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.setZoomLevel(0.6)
        ipcRenderer.send('zoom-level-set', webFrame.getZoomLevel())
      `
      w.webContents.on('did-finish-load', () => {
        if (initialNavigation) {
          w.webContents.executeJavaScript(source, () => {})
        } else {
          w.webContents.getZoomLevel((zoomLevel) => {
            assert.equal(zoomLevel, 0)
            done()
          })
        }
      })
      ipcMain.once('zoom-level-set', (e, zoomLevel) => {
        assert.equal(zoomLevel, 0.6)
        w.loadURL(`file://${fixtures}/pages/d.html`)
        initialNavigation = false
      })
      w.loadURL(`file://${fixtures}/pages/c.html`)
    })
  })

  describe('webrtc ip policy api', () => {
    it('can set and get webrtc ip policies', () => {
      const policies = [
        'default',
        'default_public_interface_only',
        'default_public_and_private_interfaces',
        'disable_non_proxied_udp'
      ]
      policies.forEach((policy) => {
        w.webContents.setWebRTCIPHandlingPolicy(policy)
        assert.equal(w.webContents.getWebRTCIPHandlingPolicy(), policy)
      })
    })
  })

  describe('will-prevent-unload event', () => {
    it('does not emit if beforeunload returns undefined', (done) => {
      w.once('closed', () => {
        done()
      })
      w.webContents.on('will-prevent-unload', (e) => {
        assert.fail('should not have fired')
      })
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-undefined.html'))
    })

    it('emits if beforeunload returns false', (done) => {
      w.webContents.on('will-prevent-unload', () => {
        done()
      })
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html'))
    })

    it('supports calling preventDefault on will-prevent-unload events', (done) => {
      ipcRenderer.send('prevent-next-will-prevent-unload', w.webContents.id)
      w.once('closed', () => done())
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html'))
    })
  })

  describe('setIgnoreMenuShortcuts(ignore)', () => {
    it('does not throw', () => {
      assert.equal(w.webContents.setIgnoreMenuShortcuts(true), undefined)
      assert.equal(w.webContents.setIgnoreMenuShortcuts(false), undefined)
    })
  })

  // Destroying webContents in its event listener is going to crash when
  // Electron is built in Debug mode.
  xdescribe('destroy()', () => {
    let server

    before((done) => {
      server = http.createServer((request, response) => {
        switch (request.url) {
          case '/404':
            response.statusCode = '404'
            response.end()
            break
          case '/301':
            response.statusCode = '301'
            response.setHeader('Location', '/200')
            response.end()
            break
          case '/200':
            response.statusCode = '200'
            response.end('hello')
            break
          default:
            done('unsupported endpoint')
        }
      }).listen(0, '127.0.0.1', () => {
        server.url = 'http://127.0.0.1:' + server.address().port
        done()
      })
    })

    after(() => {
      server.close()
      server = null
    })

    it('should not crash when invoked synchronously inside navigation observer', (done) => {
      const events = [
        { name: 'did-start-loading', url: `${server.url}/200` },
        { name: 'dom-ready', url: `${server.url}/200` },
        { name: 'did-stop-loading', url: `${server.url}/200` },
        { name: 'did-finish-load', url: `${server.url}/200` },
        // FIXME: Multiple Emit calls inside an observer assume that object
        // will be alive till end of the observer. Synchronous `destroy` api
        // violates this contract and crashes.
        // { name: 'did-frame-finish-load', url: `${server.url}/200` },
        { name: 'did-fail-load', url: `${server.url}/404` }
      ]
      const responseEvent = 'webcontents-destroyed'

      function * genNavigationEvent () {
        let eventOptions = null
        while ((eventOptions = events.shift()) && events.length) {
          eventOptions.responseEvent = responseEvent
          ipcRenderer.send('test-webcontents-navigation-observer', eventOptions)
          yield 1
        }
      }

      let gen = genNavigationEvent()
      ipcRenderer.on(responseEvent, () => {
        if (!gen.next().value) done()
      })
      gen.next()
    })
  })

  describe('did-change-theme-color event', () => {
    it('is triggered with correct theme color', (done) => {
      let count = 0
      w.webContents.on('did-change-theme-color', (e, color) => {
        if (count === 0) {
          count += 1
          assert.equal(color, '#FFEEDD')
          w.loadURL(`file://${path.join(__dirname, 'fixtures', 'pages', 'base-page.html')}`)
        } else if (count === 1) {
          assert.equal(color, null)
          done()
        }
      })
      w.loadURL(`file://${path.join(__dirname, 'fixtures', 'pages', 'theme-color.html')}`)
    })
  })

  describe('console-message event', () => {
    it('is triggered with correct log message', (done) => {
      w.webContents.on('console-message', (e, level, message) => {
        // Don't just assert as Chromium might emit other logs that we should ignore.
        if (message === 'a') {
          done()
        }
      })
      w.loadURL(`file://${fixtures}/pages/a.html`)
    })
  })

  describe('referrer', () => {
    it('propagates referrer information to new target=_blank windows', (done) => {
      const server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          assert.equal(req.headers.referer, 'http://127.0.0.1:' + server.address().port + '/')
          return done()
        }
        res.end('<a id="a" href="/should_have_referrer" target="_blank">link</a>')
      })
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + server.address().port + '/'
        w.webContents.once('did-finish-load', () => {
          w.webContents.once('new-window', (event, newUrl, frameName, disposition, options, features, referrer) => {
            assert.equal(referrer.url, url)
            assert.equal(referrer.policy, 'no-referrer-when-downgrade')
          })
          w.webContents.executeJavaScript('a.click()')
        })
        w.loadURL(url)
      })
    })

    // TODO(jeremy): window.open() in a real browser passes the referrer, but
    // our hacked-up window.open() shim doesn't. It should.
    xit('propagates referrer information to windows opened with window.open', (done) => {
      const server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          assert.equal(req.headers.referer, 'http://127.0.0.1:' + server.address().port + '/')
          return done()
        }
        res.end('')
      })
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + server.address().port + '/'
        w.webContents.once('did-finish-load', () => {
          w.webContents.once('new-window', (event, newUrl, frameName, disposition, options, features, referrer) => {
            assert.equal(referrer.url, url)
            assert.equal(referrer.policy, 'no-referrer-when-downgrade')
          })
          w.webContents.executeJavaScript('window.open(location.href + "should_have_referrer")')
        })
        w.loadURL(url)
      })
    })
  })

  describe('webframe messages in sandboxed contents', () => {
    it('responds to executeJavaScript', (done) => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })
      w.webContents.once('did-finish-load', () => {
        w.webContents.executeJavaScript('37 + 5', (result) => {
          assert.equal(result, 42)
          done()
        })
      })
      w.loadURL('about:blank')
    })
  })
})
