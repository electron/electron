const assert = require('assert')
const { screen } = require('electron').remote

describe('screen module', () => {
  describe('screen.getCursorScreenPoint()', () => {
    it('returns a point object', () => {
      const point = screen.getCursorScreenPoint()
      assert.strictEqual(typeof point.x, 'number')
      assert.strictEqual(typeof point.y, 'number')
    })
  })

  describe('screen.getPrimaryDisplay()', () => {
    it('returns a display object', () => {
      const display = screen.getPrimaryDisplay()
      assert.strictEqual(typeof display.scaleFactor, 'number')
      assert(display.size.width > 0)
      assert(display.size.height > 0)
    })
  })
})
