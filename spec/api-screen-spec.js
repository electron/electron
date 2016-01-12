var assert, screen;

assert = require('assert');

screen = require('electron').screen;

describe('screen module', function() {
  describe('screen.getCursorScreenPoint()', function() {
    return it('returns a point object', function() {
      var point;
      point = screen.getCursorScreenPoint();
      assert.equal(typeof point.x, 'number');
      return assert.equal(typeof point.y, 'number');
    });
  });
  return describe('screen.getPrimaryDisplay()', function() {
    return it('returns a display object', function() {
      var display;
      display = screen.getPrimaryDisplay();
      assert.equal(typeof display.scaleFactor, 'number');
      assert(display.size.width > 0);
      return assert(display.size.height > 0);
    });
  });
});
