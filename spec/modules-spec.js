const assert = require('assert');
const path = require('path');
const temp = require('temp');

describe('third-party module', function() {
  var fixtures;
  fixtures = path.join(__dirname, 'fixtures');
  temp.track();
  if (process.platform !== 'win32' || process.execPath.toLowerCase().indexOf('\\out\\d\\') === -1) {
    describe('runas', function() {
      it('can be required in renderer', function() {
        return require('runas');
      });
      return it('can be required in node binary', function(done) {
        var child, runas;
        runas = path.join(fixtures, 'module', 'runas.js');
        child = require('child_process').fork(runas);
        return child.on('message', function(msg) {
          assert.equal(msg, 'ok');
          return done();
        });
      });
    });
    describe('ffi', function() {
      return it('does not crash', function() {
        var ffi, libm;
        ffi = require('ffi');
        libm = ffi.Library('libm', {
          ceil: ['double', ['double']]
        });
        return assert.equal(libm.ceil(1.5), 2);
      });
    });
  }
  return describe('q', function() {
    var Q;
    Q = require('q');
    return describe('Q.when', function() {
      return it('emits the fullfil callback', function(done) {
        return Q(true).then(function(val) {
          assert.equal(val, true);
          return done();
        });
      });
    });
  });
});
