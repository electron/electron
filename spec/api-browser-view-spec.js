'use strict'

const assert = require('assert')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {BrowserView, BrowserWindow} = remote

describe('BrowserView module', function () {
  var w = null
  var view = null

  beforeEach(function () {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false
      }
    })
  })

  afterEach(function () {
    if (view) {
      view.destroy()
      view = null
    }

    return closeWindow(w).then(function () { w = null })
  })

  describe('BrowserView.setBackgroundColor()', function () {
    it('does not throw for valid args', function () {
      view = new BrowserView()
      view.setBackgroundColor('#000')
    })

    it('throws for invalid args', function () {
      view = new BrowserView()
      assert.throws(function () {
        view.setBackgroundColor(null)
      }, /conversion failure/)
    })
  })

  describe('BrowserView.setAutoResize()', function () {
    it('does not throw for valid args', function () {
      view = new BrowserView()
      view.setAutoResize({})
      view.setAutoResize({ width: true, height: false })
    })

    it('throws for invalid args', function () {
      view = new BrowserView()
      assert.throws(function () {
        view.setAutoResize(null)
      }, /conversion failure/)
    })
  })

  describe('BrowserView.setBounds()', function () {
    it('does not throw for valid args', function () {
      view = new BrowserView()
      view.setBounds({ x: 0, y: 0, width: 1, height: 1 })
    })

    it('throws for invalid args', function () {
      view = new BrowserView()
      assert.throws(function () {
        view.setBounds(null)
      }, /conversion failure/)
      assert.throws(function () {
        view.setBounds({})
      }, /conversion failure/)
    })
  })

  describe('BrowserWindow.setBrowserView()', function () {
    it('does not throw for valid args', function () {
      view = new BrowserView()
      w.setBrowserView(view)
    })

    it('does not throw if called multiple times with same view', function () {
      view = new BrowserView()
      w.setBrowserView(view)
      w.setBrowserView(view)
      w.setBrowserView(view)
    })
  })

  describe('BrowserView.webContents.getOwnerBrowserWindow()', function () {
    it('points to owning window', function () {
      view = new BrowserView()
      assert.ok(!view.webContents.getOwnerBrowserWindow())
      w.setBrowserView(view)
      assert.equal(view.webContents.getOwnerBrowserWindow(), w)
      w.setBrowserView(null)
      assert.ok(!view.webContents.getOwnerBrowserWindow())
    })
  })

  describe('BrowserView.fromId()', function () {
    it('returns the view with given id', function () {
      view = new BrowserView()
      w.setBrowserView(view)
      assert.notEqual(view.id, null)
      let view2 = BrowserView.fromId(view.id)
      assert.equal(view2.webContents.id, view.webContents.id)
    })
  })
})
