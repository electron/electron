'use strict'

const assert = require('assert')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {BrowserView, BrowserWindow} = remote

describe('BrowserView module', () => {
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

  describe('BrowserView.setBackgroundColor()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView()
      view.setBackgroundColor('#000')
    })

    it('throws for invalid args', () => {
      view = new BrowserView()
      assert.throws(() => {
        view.setBackgroundColor(null)
      }, /conversion failure/)
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
      assert.throws(() => {
        view.setAutoResize(null)
      }, /conversion failure/)
    })
  })

  describe('BrowserView.setBounds()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView()
      view.setBounds({ x: 0, y: 0, width: 1, height: 1 })
    })

    it('throws for invalid args', () => {
      view = new BrowserView()
      assert.throws(() => {
        view.setBounds(null)
      }, /conversion failure/)
      assert.throws(() => {
        view.setBounds({})
      }, /conversion failure/)
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
      assert.notEqual(view.id, null)
      let view2 = w.getBrowserView()
      assert.equal(view2.webContents.id, view.webContents.id)
    })

    it('returns null if none is set', () => {
      let view = w.getBrowserView()
      assert.equal(null, view)
    })
  })

  describe('BrowserView.webContents.getOwnerBrowserWindow()', () => {
    it('points to owning window', () => {
      view = new BrowserView()
      assert.ok(!view.webContents.getOwnerBrowserWindow())
      w.setBrowserView(view)
      assert.equal(view.webContents.getOwnerBrowserWindow(), w)
      w.setBrowserView(null)
      assert.ok(!view.webContents.getOwnerBrowserWindow())
    })
  })

  describe('BrowserView.fromId()', () => {
    it('returns the view with given id', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      assert.notEqual(view.id, null)
      let view2 = BrowserView.fromId(view.id)
      assert.equal(view2.webContents.id, view.webContents.id)
    })
  })

  describe('BrowserView.fromWebContents()', () => {
    it('returns the view with given id', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      assert.notEqual(view.id, null)
      let view2 = BrowserView.fromWebContents(view.webContents)
      assert.equal(view2.webContents.id, view.webContents.id)
    })
  })

  describe('BrowserView.getAllViews()', () => {
    it('returns all views', () => {
      view = new BrowserView()
      w.setBrowserView(view)
      assert.notEqual(view.id, null)

      const views = BrowserView.getAllViews()
      assert.equal(views.length, 1)
      assert.equal(views[0].webContents.id, view.webContents.id)
    })
  })
})
