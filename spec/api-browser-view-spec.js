'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {BrowserView, BrowserWindow} = remote

const {expect} = chai
chai.use(dirtyChai)

const createBrowserView = () =>
  new BrowserView({
    webPreferences: {
      nodeIntegration: true
    }
  })

describe('BrowserView module', () => {
  let w = null
  let view = null

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false,
        nodeIntegration: true
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
      view = createBrowserView()
      view.destroy()
    })
  })

  describe('BrowserView.isDestroyed()', () => {
    it('returns correct value', () => {
      view = createBrowserView()
      expect(view.isDestroyed()).to.be.false()
      view.destroy()
      expect(view.isDestroyed()).to.be.true()
    })
  })

  describe('BrowserView.setBackgroundColor()', () => {
    it('does not throw for valid args', () => {
      view = createBrowserView()
      view.setBackgroundColor('#000')
    })

    it('throws for invalid args', () => {
      view = createBrowserView()
      expect(() => {
        view.setBackgroundColor(null)
      }).to.throw(/conversion failure/)
    })
  })

  describe('BrowserView.setAutoResize()', () => {
    it('does not throw for valid args', () => {
      view = createBrowserView()
      view.setAutoResize({})
      view.setAutoResize({ width: true, height: false })
    })

    it('throws for invalid args', () => {
      view = createBrowserView()
      expect(() => {
        view.setAutoResize(null)
      }).to.throw(/conversion failure/)
    })
  })

  describe('BrowserView.setBounds()', () => {
    it('does not throw for valid args', () => {
      view = createBrowserView()
      view.setBounds({ x: 0, y: 0, width: 1, height: 1 })
    })

    it('throws for invalid args', () => {
      view = createBrowserView()
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
      view = createBrowserView()
      w.setBrowserView(view)
    })

    it('does not throw if called multiple times with same view', () => {
      view = createBrowserView()
      w.setBrowserView(view)
      w.setBrowserView(view)
      w.setBrowserView(view)
    })
  })

  describe('BrowserWindow.getBrowserView()', () => {
    it('returns the set view', () => {
      view = createBrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      let view2 = w.getBrowserView()
      expect(view2.webContents.id).to.equal(view.webContents.id)
    })

    it('returns null if none is set', () => {
      let view = w.getBrowserView()
      expect(view).to.be.null()
    })
  })

  describe('BrowserView.webContents.getOwnerBrowserWindow()', () => {
    it('points to owning window', () => {
      view = createBrowserView()
      expect(view.webContents.getOwnerBrowserWindow()).to.be.null()

      w.setBrowserView(view)
      expect(view.webContents.getOwnerBrowserWindow()).to.equal(w)

      w.setBrowserView(null)
      expect(view.webContents.getOwnerBrowserWindow()).to.be.null()
    })
  })

  describe('BrowserView.fromId()', () => {
    it('returns the view with given id', () => {
      view = createBrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      let view2 = BrowserView.fromId(view.id)
      expect(view2.webContents.id).to.equal(view.webContents.id)
    })
  })

  describe('BrowserView.fromWebContents()', () => {
    it('returns the view with given id', () => {
      view = createBrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      let view2 = BrowserView.fromWebContents(view.webContents)
      expect(view2.webContents.id).to.equal(view.webContents.id)
    })
  })

  describe('BrowserView.getAllViews()', () => {
    it('returns all views', () => {
      view = createBrowserView()
      w.setBrowserView(view)
      expect(view.id).to.not.be.null()

      const views = BrowserView.getAllViews()
      expect(views).to.be.an('array').that.has.lengthOf(1)
      expect(views[0].webContents.id).to.equal(view.webContents.id)
    })
  })
})
