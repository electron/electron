const assert = require('assert');
const desktopCapturer = require('electron').desktopCapturer;

describe('desktopCapturer', function() {
  it('should return a non-empty array of sources', function(done) {
    desktopCapturer.getSources({
      types: ['window', 'screen']
    }, function(error, sources) {
      assert.equal(error, null);
      assert.notEqual(sources.length, 0);
      done();
    });
  });
});
