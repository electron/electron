const assert = require('assert')
const screen = require('electron').screen

describe('screen module', function () {
  describe('screen.getCursorScreenPoint()', function () {
    it('returns a point object', function () {
      var point = screen.getCursorScreenPoint()
      assert.equal(typeof point.x, 'number')
      assert.equal(typeof point.y, 'number')
    })
  })

  describe('screen.getPrimaryDisplay()', function () {
    it('returns a display object', function () {
      var display = screen.getPrimaryDisplay()
      assert.equal(typeof display.scaleFactor, 'number')
      assert(display.size.width > 0)
      assert(display.size.height > 0)
    })
  })
})
