var BrowserWindow, assert, comparePaths, ipcMain, ipcRenderer, path, ref, ref1, remote;

assert = require('assert');

path = require('path');

ref = require('electron'), ipcRenderer = ref.ipcRenderer, remote = ref.remote;

ref1 = remote.require('electron'), ipcMain = ref1.ipcMain, BrowserWindow = ref1.BrowserWindow;

comparePaths = function(path1, path2) {
  if (process.platform === 'win32') {
    path1 = path1.toLowerCase();
    path2 = path2.toLowerCase();
  }
  return assert.equal(path1, path2);
};

describe('ipc module', function() {
  var fixtures;
  fixtures = path.join(__dirname, 'fixtures');
  describe('remote.require', function() {
    it('should returns same object for the same module', function() {
      var dialog1, dialog2;
      dialog1 = remote.require('electron');
      dialog2 = remote.require('electron');
      return assert.equal(dialog1, dialog2);
    });
    it('should work when object contains id property', function() {
      var a;
      a = remote.require(path.join(fixtures, 'module', 'id.js'));
      return assert.equal(a.id, 1127);
    });
    return it('should search module from the user app', function() {
      comparePaths(path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js'));
      return comparePaths(path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules'));
    });
  });
  describe('remote.createFunctionWithReturnValue', function() {
    return it('should be called in browser synchronously', function() {
      var buf, call, result;
      buf = new Buffer('test');
      call = remote.require(path.join(fixtures, 'module', 'call.js'));
      result = call.call(remote.createFunctionWithReturnValue(buf));
      return assert.equal(result.constructor.name, 'Buffer');
    });
  });
  describe('remote object in renderer', function() {
    it('can change its properties', function() {
      var property, property2;
      property = remote.require(path.join(fixtures, 'module', 'property.js'));
      assert.equal(property.property, 1127);
      property.property = 1007;
      assert.equal(property.property, 1007);
      property2 = remote.require(path.join(fixtures, 'module', 'property.js'));
      assert.equal(property2.property, 1007);
      return property.property = 1127;
    });
    return it('can construct an object from its member', function() {
      var call, obj;
      call = remote.require(path.join(fixtures, 'module', 'call.js'));
      obj = new call.constructor;
      return assert.equal(obj.test, 'test');
    });
  });
  describe('remote value in browser', function() {
    var print;
    print = path.join(fixtures, 'module', 'print_name.js');
    it('keeps its constructor name for objects', function() {
      var buf, print_name;
      buf = new Buffer('test');
      print_name = remote.require(print);
      return assert.equal(print_name.print(buf), 'Buffer');
    });
    return it('supports instanceof Date', function() {
      var now, print_name;
      now = new Date();
      print_name = remote.require(print);
      assert.equal(print_name.print(now), 'Date');
      return assert.deepEqual(print_name.echo(now), now);
    });
  });
  describe('remote promise', function() {
    return it('can be used as promise in each side', function(done) {
      var promise;
      promise = remote.require(path.join(fixtures, 'module', 'promise.js'));
      return promise.twicePromise(Promise.resolve(1234)).then(function(value) {
        assert.equal(value, 2468);
        return done();
      });
    });
  });
  describe('ipc.sender.send', function() {
    return it('should work when sending an object containing id property', function(done) {
      var obj;
      obj = {
        id: 1,
        name: 'ly'
      };
      ipcRenderer.once('message', function(event, message) {
        assert.deepEqual(message, obj);
        return done();
      });
      return ipcRenderer.send('message', obj);
    });
  });
  describe('ipc.sendSync', function() {
    it('can be replied by setting event.returnValue', function() {
      var msg;
      msg = ipcRenderer.sendSync('echo', 'test');
      return assert.equal(msg, 'test');
    });
    return it('does not crash when reply is not sent and browser is destroyed', function(done) {
      var w;
      this.timeout(10000);
      w = new BrowserWindow({
        show: false
      });
      ipcMain.once('send-sync-message', function(event) {
        event.returnValue = null;
        w.destroy();
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'send-sync-message.html'));
    });
  });
  return describe('remote listeners', function() {
    var w;
    w = null;
    afterEach(function() {
      return w.destroy();
    });
    return it('can be added and removed correctly', function() {
      var listener;
      w = new BrowserWindow({
        show: false
      });
      listener = function() {};
      w.on('test', listener);
      assert.equal(w.listenerCount('test'), 1);
      w.removeListener('test', listener);
      return assert.equal(w.listenerCount('test'), 0);
    });
  });
});
