const assert = require('assert')
const {screen} = require('electron')

describe('screen module', () => {
  describe('screen.getCursorScreenPoint()', () => {
    it('returns a point object', () => {
      const point = screen.getCursorScreenPoint()
      assert.equal(typeof point.x, 'number')
      assert.equal(typeof point.y, 'number')
    })
  })

  describe('screen.getPrimaryDisplay()', () => {
    it('returns a display object', () => {
      const display = screen.getPrimaryDisplay()
      assert.equal(typeof display.scaleFactor, 'number')
      assert(display.size.width > 0)
      assert(display.size.height > 0)
    })
  })

  describe('screen.getMenuBarHeight()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('returns an integer', () => {
      const screenHeight = screen.getMenuBarHeight()
      assert.equal(typeof screenHeight, 'number')
    })
  })
})
