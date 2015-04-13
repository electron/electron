assert = require 'assert'
screen = require 'screen'

describe 'screen module', ->
  describe 'screen.getCursorScreenPoint()', ->
    it 'returns a point object', ->
      point = screen.getCursorScreenPoint()
      assert.equal typeof(point.x), 'number'
      assert.equal typeof(point.y), 'number'

  describe 'screen.getPrimaryDisplay()', ->
    it 'returns a display object', ->
      display = screen.getPrimaryDisplay()
      assert.equal typeof(display.scaleFactor), 'number'
      assert display.size.width > 0
      assert display.size.height > 0

  describe 'screen.getNumDisplays()', ->
    it 'returns the number of display objects', ->
      count = screen.getNumDisplays()
      assert.equal count, 1
