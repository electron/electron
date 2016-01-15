var assert, desktopCapturer;

assert = require('assert');

desktopCapturer = require('electron').desktopCapturer;

describe('desktopCapturer', function() {
  return it('should return a non-empty array of sources', function(done) {
    return desktopCapturer.getSources({
      types: ['window', 'screen']
    }, function(error, sources) {
      assert.equal(error, null);
      assert.notEqual(sources.length, 0);
      return done();
    });
  });
});
