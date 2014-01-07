assert = require 'assert'
screen = require 'screen'

describe 'screen module', ->
  describe 'screen.getCursorScreenPoint()', ->
    it 'returns a point object', ->
      point = screen.getCursorScreenPoint()
      assert typeof(point.x), 'number'
      assert typeof(point.y), 'number'
