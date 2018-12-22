'use strict'

const assert = require('assert')
const chai = require('chai')
const ChildProcess = require('child_process')
const dirtyChai = require('dirty-chai')
const path = require('path')
const { emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')

const { remote } = require('electron')
const { BrowserView, BrowserWindow } = remote

const { expect } = chai
chai.use(dirtyChai)

describe('BrowserView module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')

  let w = null
  let view = null

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

  afterEach(() => {
    if (view) {
      view.destroy()
      view = null
    }

    return closeWindow(w).then(() => { w = null })
  })

  describe('BrowserView.destroy()', () => {
    it('does not throw', () => {
      view = new BrowserView()
      view.destroy()
    })
  })

  describe('BrowserView.isDestroyed()', () => {
    it('returns correct value', () => {
      view = new BrowserView()
      expect(view.isDestroyed()).to.be.false()
      view.destroy()
      expect(view.isDestroyed()).to.be.true()
    })
  })

  describe('BrowserView.setBackgroundColor()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView()
      view.setBackgroundColor('#000')
    })

    it('throws for invalid args', () => {
      view = new BrowserView()
      expect(() => {
        view.setBackgroundColor(null)
      }).to.throw(/conversion failure/)
    })
  })

  describe('BrowserView.setAutoResize()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView()
      view.setAutoResize({})
      view.setAutoResize({ width: true, height: false })
    })

    it('throws for invalid args', () => {
      view = new BrowserView()
      expect(() => {
        view.setAutoResize(null)
      }).to.throw(/conversion failure/)
    })
  })

  describe('BrowserView.setBounds()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView()
      view.setBounds({ x: 0, y: 0, width: 1, height: 1 })
    })

    it('throws for invalid args', () => {
      view = new BrowserView()
      expect(() => {
        view.setBounds(null)
      }).to.throw(/conversion failure/)
      expect(() => {
        view.setBounds({})
      }).to.throw(/conversion failure/)
    })
  })

  describe('BrowserWindow.setBrowserView()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView()
      w.setBrowserView(view)
    })

    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      w.setBrowserView(view)
      w.setBrowserView(view)
    })
  })

  describe('BrowserWindow.getBrowserView()', () => {
    it('returns the set view', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      const view2 = w.getBrowserView()
      expect(view2.webContents.id).to.equal(view.webContents.id)
    })

    it('returns null if none is set', () => {
      const view = w.getBrowserView()
      expect(view).to.be.null()
    })
  })

  describe('BrowserWindow.addBrowserView()', () => {
    it('does not throw for valid args', () => {
      let view1 = new BrowserView()
      w.addBrowserView(view1)
      let view2 = new BrowserView()
      w.addBrowserView(view2)
      view1.destroy()
      view1 = null
      view2.destroy()
      view2 = null
    })
    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView()
      w.addBrowserView(view)
      w.addBrowserView(view)
      w.addBrowserView(view)
    })
  })

  describe('BrowserWindow.removeBrowserView()', () => {
    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView()
      w.addBrowserView(view)
      w.removeBrowserView(view)
      w.removeBrowserView(view)
    })
  })

  describe('BrowserWindow.getBrowserViews()', () => {
    it('returns same views as was added', () => {
      let view1 = new BrowserView()
      w.addBrowserView(view1)
      let view2 = new BrowserView()
      w.addBrowserView(view2)

      expect(view1.id).to.be.not.null()
      const views = w.getBrowserViews()
      expect(views.length).to.equal(2)
      expect(views[0].webContents.id).to.equal(view1.webContents.id)
      expect(views[1].webContents.id).to.equal(view2.webContents.id)

      view1.destroy()
      view1 = null
      view2.destroy()
      view2 = null
    })
  })

  describe('BrowserView.webContents.getOwnerBrowserWindow()', () => {
    it('points to owning window', () => {
      view = new BrowserView()
      expect(view.webContents.getOwnerBrowserWindow()).to.be.null()

      w.setBrowserView(view)
      expect(view.webContents.getOwnerBrowserWindow()).to.equal(w)

      w.setBrowserView(null)
      expect(view.webContents.getOwnerBrowserWindow()).to.be.null()
    })
  })

  describe('BrowserView.fromId()', () => {
    it('returns the view with given id', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      const view2 = BrowserView.fromId(view.id)
      expect(view2.webContents.id).to.equal(view.webContents.id)
    })
  })

  describe('BrowserView.fromWebContents()', () => {
    it('returns the view with given id', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      const view2 = BrowserView.fromWebContents(view.webContents)
      expect(view2.webContents.id).to.equal(view.webContents.id)
    })
  })

  describe('BrowserView.getAllViews()', () => {
    it('returns all views', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      const views = BrowserView.getAllViews()
      expect(views).to.be.an('array').that.has.lengthOf(1)
      expect(views[0].webContents.id).to.equal(view.webContents.id)
    })
  })

  describe('new BrowserView()', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(fixtures, 'api', 'leak-exit-browserview.js')
      const electronPath = remote.getGlobal('process').execPath
      const appProcess = ChildProcess.spawn(electronPath, [appPath])
      const [code] = await emittedOnce(appProcess, 'close')
      expect(code).to.equal(0)
    })
  })

  describe('window.open()', () => {
    it('works in BrowserView', (done) => {
      view = new BrowserView()
      w.setBrowserView(view)
      view.webContents.once('new-window', (e, url, frameName, disposition, options, additionalFeatures) => {
        e.preventDefault()
        assert.strictEqual(url, 'http://host/')
        assert.strictEqual(frameName, 'host')
        assert.strictEqual(additionalFeatures[0], 'this-is-not-a-standard-feature')
        done()
      })
      view.webContents.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
    })
  })
})
