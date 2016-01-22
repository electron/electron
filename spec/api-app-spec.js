var BrowserWindow, ChildProcess, app, assert, path, ref, remote;

assert = require('assert');

ChildProcess = require('child_process');

path = require('path');

remote = require('electron').remote;

ref = remote.require('electron'), app = ref.app, BrowserWindow = ref.BrowserWindow;

describe('app module', function() {
  describe('app.getVersion()', function() {
    return it('returns the version field of package.json', function() {
      return assert.equal(app.getVersion(), '0.1.0');
    });
  });
  describe('app.setVersion(version)', function() {
    return it('overrides the version', function() {
      assert.equal(app.getVersion(), '0.1.0');
      app.setVersion('test-version');
      assert.equal(app.getVersion(), 'test-version');
      return app.setVersion('0.1.0');
    });
  });
  describe('app.getName()', function() {
    return it('returns the name field of package.json', function() {
      return assert.equal(app.getName(), 'Electron Test');
    });
  });
  describe('app.setName(name)', function() {
    return it('overrides the name', function() {
      assert.equal(app.getName(), 'Electron Test');
      app.setName('test-name');
      assert.equal(app.getName(), 'test-name');
      return app.setName('Electron Test');
    });
  });
  describe('app.getLocale()', function() {
    return it('should not be empty', function() {
      return assert.notEqual(app.getLocale(), '');
    });
  });
  describe('app.exit(exitCode)', function() {
    var appProcess;
    appProcess = null;
    afterEach(function() {
      return appProcess != null ? appProcess.kill() : void 0;
    });
    return it('emits a process exit event with the code', function(done) {
      var appPath, electronPath, output;
      appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app');
      electronPath = remote.getGlobal('process').execPath;
      appProcess = ChildProcess.spawn(electronPath, [appPath]);
      output = '';
      appProcess.stdout.on('data', function(data) {
        return output += data;
      });
      return appProcess.on('close', function(code) {
        if (process.platform !== 'win32') {
          assert.notEqual(output.indexOf('Exit event with code: 123'), -1);
        }
        assert.equal(code, 123);
        return done();
      });
    });
  });
  return describe('BrowserWindow events', function() {
    var w;
    w = null;
    afterEach(function() {
      if (w != null) {
        w.destroy();
      }
      return w = null;
    });
    it('should emit browser-window-focus event when window is focused', function(done) {
      app.once('browser-window-focus', function(e, window) {
        assert.equal(w.id, window.id);
        return done();
      });
      w = new BrowserWindow({
        show: false
      });
      return w.emit('focus');
    });
    it('should emit browser-window-blur event when window is blured', function(done) {
      app.once('browser-window-blur', function(e, window) {
        assert.equal(w.id, window.id);
        return done();
      });
      w = new BrowserWindow({
        show: false
      });
      return w.emit('blur');
    });
    return it('should emit browser-window-created event when window is created', function(done) {
      app.once('browser-window-created', function(e, window) {
        return setImmediate(function() {
          assert.equal(w.id, window.id);
          return done();
        });
      });
      w = new BrowserWindow({
        show: false
      });
      return w.emit('blur');
    });
  });
});
