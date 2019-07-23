import { expect } from 'chai'
import { Menu, Tray, nativeImage } from 'electron'

describe('tray module', () => {
  let tray: Tray;

  beforeEach(() => {
    tray = new Tray(nativeImage.createEmpty())
  })

  afterEach(() => {
    tray = null as any
  })

  describe('tray get/set ignoreDoubleClickEvents', () => {
    before(function () {
      if (process.platform !== 'darwin') this.skip()
    })

    it('returns false by default', () => {
      const ignored = tray.getIgnoreDoubleClickEvents()
      expect(ignored).to.be.false
    })

    it('can be set to true', () => {
      tray.setIgnoreDoubleClickEvents(true)

      const ignored = tray.getIgnoreDoubleClickEvents()
      expect(ignored).to.be.true
    })
  })

  describe('tray.setContextMenu(menu)', () => {
    it ('accepts both null and Menu as parameters', () => {
      expect(() => { tray.setContextMenu(new Menu()) }).to.not.throw()
      expect(() => { tray.setContextMenu(null) }).to.not.throw()

      tray.destroy()
    })
  })

  describe('tray.destroy()', () => {
    it('destroys a tray', () => {
      expect(tray.isDestroyed()).to.be.false('tray should not be destroyed')
      tray.destroy()

      expect(tray.isDestroyed()).to.be.true('tray should be destroyed')
    })
  })

  describe('tray.popUpContextMenu()', () => {
    it('can be called when menu is showing', function (done) {
      if (process.platform !== 'win32') this.skip()

      tray.setContextMenu(Menu.buildFromTemplate([{ label: 'Test' }]))
      setTimeout(() => {
        tray.popUpContextMenu()
        done()
      })
      tray.popUpContextMenu()

      tray.destroy()
    })
  })

  describe('tray.getBounds()', () => {
    afterEach(() => { tray.destroy() })

    it ('returns a bounds object', function () {
      if (process.platform === 'linux') this.skip()

      const bounds = tray.getBounds()
      expect(bounds).to.be.an('object').and.to.have.all.keys('x', 'y', 'width', 'height');
    })
  })

  describe('tray.setImage(image)', () => {
    it('accepts empty image', () => {
      tray.setImage(nativeImage.createEmpty())

      tray.destroy()
    })
  })

  describe('tray.setPressedImage(image)', () => {
    it('accepts empty image', () => {
      tray.setPressedImage(nativeImage.createEmpty())

      tray.destroy()
    })
  })

  describe('tray get/set title', () => {
    before(function () {
      if (process.platform !== 'darwin') this.skip()
    })

    afterEach(() => { tray.destroy() })

    it('sets/gets non-empty title', () => {
      const title = 'Hello World!'
      tray.setTitle(title)
      const newTitle = tray.getTitle()

      expect(newTitle).to.equal(title)
    })

    it('sets/gets empty title', () => {
      const title = ''
      tray.setTitle(title)
      const newTitle = tray.getTitle()

      expect(newTitle).to.equal(title)
    })
  })
})
