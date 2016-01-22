var assert, path, webFrame;

assert = require('assert');

path = require('path');

webFrame = require('electron').webFrame;

describe('webFrame module', function() {
  var fixtures;
  fixtures = path.resolve(__dirname, 'fixtures');
  return describe('webFrame.registerURLSchemeAsPrivileged', function() {
    return it('supports fetch api', function(done) {
      var url;
      webFrame.registerURLSchemeAsPrivileged('file');
      url = "file://" + fixtures + "/assets/logo.png";
      return fetch(url).then(function(response) {
        assert(response.ok);
        return done();
      })["catch"](function(err) {
        return done('unexpected error : ' + err);
      });
    });
  });
});
