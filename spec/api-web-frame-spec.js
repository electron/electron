const assert = require('assert');
const path = require('path');
const webFrame = require('electron').webFrame;

describe('webFrame module', function() {
  var fixtures = path.resolve(__dirname, 'fixtures');
  return describe('webFrame.registerURLSchemeAsPrivileged', function() {
    return it('supports fetch api', function(done) {
      webFrame.registerURLSchemeAsPrivileged('file');
      var url = "file://" + fixtures + "/assets/logo.png";
      return fetch(url).then(function(response) {
        assert(response.ok);
        return done();
      })["catch"](function(err) {
        return done('unexpected error : ' + err);
      });
    });
  });
});
