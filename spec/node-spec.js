const assert = require('assert');
const child_process = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');
const remote = require('electron').remote;

describe('node feature', function() {
  var fixtures;
  fixtures = path.join(__dirname, 'fixtures');
  describe('child_process', function() {
    return describe('child_process.fork', function() {
      it('works in current process', function(done) {
        var child;
        child = child_process.fork(path.join(fixtures, 'module', 'ping.js'));
        child.on('message', function(msg) {
          assert.equal(msg, 'message');
          return done();
        });
        return child.send('message');
      });
      it('preserves args', function(done) {
        var args, child;
        args = ['--expose_gc', '-test', '1'];
        child = child_process.fork(path.join(fixtures, 'module', 'process_args.js'), args);
        child.on('message', function(msg) {
          assert.deepEqual(args, msg.slice(2));
          return done();
        });
        return child.send('message');
      });
      it('works in forked process', function(done) {
        var child;
        child = child_process.fork(path.join(fixtures, 'module', 'fork_ping.js'));
        child.on('message', function(msg) {
          assert.equal(msg, 'message');
          return done();
        });
        return child.send('message');
      });
      it('works in forked process when options.env is specifed', function(done) {
        var child;
        child = child_process.fork(path.join(fixtures, 'module', 'fork_ping.js'), [], {
          path: process.env['PATH']
        });
        child.on('message', function(msg) {
          assert.equal(msg, 'message');
          return done();
        });
        return child.send('message');
      });
      it('works in browser process', function(done) {
        var child, fork;
        fork = remote.require('child_process').fork;
        child = fork(path.join(fixtures, 'module', 'ping.js'));
        child.on('message', function(msg) {
          assert.equal(msg, 'message');
          return done();
        });
        return child.send('message');
      });
      it('has String::localeCompare working in script', function(done) {
        var child;
        child = child_process.fork(path.join(fixtures, 'module', 'locale-compare.js'));
        child.on('message', function(msg) {
          assert.deepEqual(msg, [0, -1, 1]);
          return done();
        });
        return child.send('message');
      });
      return it('has setImmediate working in script', function(done) {
        var child;
        child = child_process.fork(path.join(fixtures, 'module', 'set-immediate.js'));
        child.on('message', function(msg) {
          assert.equal(msg, 'ok');
          return done();
        });
        return child.send('message');
      });
    });
  });
  describe('contexts', function() {
    describe('setTimeout in fs callback', function() {
      if (process.env.TRAVIS === 'true') {
        return;
      }
      return it('does not crash', function(done) {
        return fs.readFile(__filename, function() {
          return setTimeout(done, 0);
        });
      });
    });
    describe('throw error in node context', function() {
      return it('gets caught', function(done) {
        var error, lsts;
        error = new Error('boo!');
        lsts = process.listeners('uncaughtException');
        process.removeAllListeners('uncaughtException');
        process.on('uncaughtException', function() {
          var i, len, lst;
          process.removeAllListeners('uncaughtException');
          for (i = 0, len = lsts.length; i < len; i++) {
            lst = lsts[i];
            process.on('uncaughtException', lst);
          }
          return done();
        });
        return fs.readFile(__filename, function() {
          throw error;
        });
      });
    });
    describe('setTimeout called under Chromium event loop in browser process', function() {
      return it('can be scheduled in time', function(done) {
        return remote.getGlobal('setTimeout')(done, 0);
      });
    });
    return describe('setInterval called under Chromium event loop in browser process', function() {
      return it('can be scheduled in time', function(done) {
        var clear, interval;
        clear = function() {
          remote.getGlobal('clearInterval')(interval);
          return done();
        };
        return interval = remote.getGlobal('setInterval')(clear, 10);
      });
    });
  });
  describe('message loop', function() {
    describe('process.nextTick', function() {
      it('emits the callback', function(done) {
        return process.nextTick(done);
      });
      return it('works in nested calls', function(done) {
        return process.nextTick(function() {
          return process.nextTick(function() {
            return process.nextTick(done);
          });
        });
      });
    });
    return describe('setImmediate', function() {
      it('emits the callback', function(done) {
        return setImmediate(done);
      });
      return it('works in nested calls', function(done) {
        return setImmediate(function() {
          return setImmediate(function() {
            return setImmediate(done);
          });
        });
      });
    });
  });
  describe('net.connect', function() {
    if (process.platform !== 'darwin') {
      return;
    }
    return it('emit error when connect to a socket path without listeners', function(done) {
      var child, script, socketPath;
      socketPath = path.join(os.tmpdir(), 'atom-shell-test.sock');
      script = path.join(fixtures, 'module', 'create_socket.js');
      child = child_process.fork(script, [socketPath]);
      return child.on('exit', function(code) {
        var client;
        assert.equal(code, 0);
        client = require('net').connect(socketPath);
        return client.on('error', function(error) {
          assert.equal(error.code, 'ECONNREFUSED');
          return done();
        });
      });
    });
  });
  describe('Buffer', function() {
    it('can be created from WebKit external string', function() {
      var b, p;
      p = document.createElement('p');
      p.innerText = '闲云潭影日悠悠，物换星移几度秋';
      b = new Buffer(p.innerText);
      assert.equal(b.toString(), '闲云潭影日悠悠，物换星移几度秋');
      return assert.equal(Buffer.byteLength(p.innerText), 45);
    });
    return it('correctly parses external one-byte UTF8 string', function() {
      var b, p;
      p = document.createElement('p');
      p.innerText = 'Jøhänñéß';
      b = new Buffer(p.innerText);
      assert.equal(b.toString(), 'Jøhänñéß');
      return assert.equal(Buffer.byteLength(p.innerText), 13);
    });
  });
  describe('process.stdout', function() {
    it('should not throw exception', function() {
      return process.stdout;
    });
    return xit('should have isTTY defined', function() {
      return assert.equal(typeof process.stdout.isTTY, 'boolean');
    });
  });
  return describe('vm.createContext', function() {
    return it('should not crash', function() {
      return require('vm').runInNewContext('');
    });
  });
});
