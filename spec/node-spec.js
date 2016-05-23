const assert = require('assert')
const child_process = require('child_process')
const fs = require('fs')
const path = require('path')
const os = require('os')
const {remote} = require('electron')
const {BrowserWindow, ipcMain} = remote

describe('node feature', function () {
  var fixtures = path.join(__dirname, 'fixtures')

  describe('child_process', function () {
    describe('child_process.fork', function () {
      it('works in current process', function (done) {
        var child = child_process.fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', function (msg) {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('preserves args', function (done) {
        var args = ['--expose_gc', '-test', '1']
        var child = child_process.fork(path.join(fixtures, 'module', 'process_args.js'), args)
        child.on('message', function (msg) {
          assert.deepEqual(args, msg.slice(2))
          done()
        })
        child.send('message')
      })

      it('works in forked process', function (done) {
        var child = child_process.fork(path.join(fixtures, 'module', 'fork_ping.js'))
        child.on('message', function (msg) {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('works in forked process when options.env is specifed', function (done) {
        var child = child_process.fork(path.join(fixtures, 'module', 'fork_ping.js'), [], {
          path: process.env['PATH']
        })
        child.on('message', function (msg) {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('works in browser process', function (done) {
        var fork = remote.require('child_process').fork
        var child = fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', function (msg) {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('has String::localeCompare working in script', function (done) {
        var child = child_process.fork(path.join(fixtures, 'module', 'locale-compare.js'))
        child.on('message', function (msg) {
          assert.deepEqual(msg, [0, -1, 1])
          done()
        })
        child.send('message')
      })

      it('has setImmediate working in script', function (done) {
        var child = child_process.fork(path.join(fixtures, 'module', 'set-immediate.js'))
        child.on('message', function (msg) {
          assert.equal(msg, 'ok')
          done()
        })
        child.send('message')
      })
    })
  })

  describe('contexts', function () {
    describe('setTimeout in fs callback', function () {
      if (process.env.TRAVIS === 'true') {
        return
      }

      it('does not crash', function (done) {
        fs.readFile(__filename, function () {
          setTimeout(done, 0)
        })
      })
    })

    describe('throw error in node context', function () {
      it('gets caught', function (done) {
        var error = new Error('boo!')
        var lsts = process.listeners('uncaughtException')
        process.removeAllListeners('uncaughtException')
        process.on('uncaughtException', function () {
          var i, len, lst
          process.removeAllListeners('uncaughtException')
          for (i = 0, len = lsts.length; i < len; i++) {
            lst = lsts[i]
            process.on('uncaughtException', lst)
          }
          done()
        })
        fs.readFile(__filename, function () {
          throw error
        })
      })
    })

    describe('setTimeout called under Chromium event loop in browser process', function () {
      it('can be scheduled in time', function (done) {
        remote.getGlobal('setTimeout')(done, 0)
      })
    })

    describe('setInterval called under Chromium event loop in browser process', function () {
      it('can be scheduled in time', function (done) {
        var clear, interval
        clear = function () {
          remote.getGlobal('clearInterval')(interval)
          done()
        }
        interval = remote.getGlobal('setInterval')(clear, 10)
      })
    })
  })

  describe('message loop', function () {
    describe('process.nextTick', function () {
      it('emits the callback', function (done) {
        process.nextTick(done)
      })

      it('works in nested calls', function (done) {
        process.nextTick(function () {
          process.nextTick(function () {
            process.nextTick(done)
          })
        })
      })
    })

    describe('setImmediate', function () {
      it('emits the callback', function (done) {
        setImmediate(done)
      })

      it('works in nested calls', function (done) {
        setImmediate(function () {
          setImmediate(function () {
            setImmediate(done)
          })
        })
      })
    })
  })

  describe('net.connect', function () {
    if (process.platform !== 'darwin') {
      return
    }

    it('emit error when connect to a socket path without listeners', function (done) {
      var socketPath = path.join(os.tmpdir(), 'atom-shell-test.sock')
      var script = path.join(fixtures, 'module', 'create_socket.js')
      var child = child_process.fork(script, [socketPath])
      child.on('exit', function (code) {
        assert.equal(code, 0)
        var client = require('net').connect(socketPath)
        client.on('error', function (error) {
          assert.equal(error.code, 'ECONNREFUSED')
          done()
        })
      })
    })
  })

  describe('Buffer', function () {
    it('can be created from WebKit external string', function () {
      var p = document.createElement('p')
      p.innerText = '闲云潭影日悠悠，物换星移几度秋'
      var b = new Buffer(p.innerText)
      assert.equal(b.toString(), '闲云潭影日悠悠，物换星移几度秋')
      assert.equal(Buffer.byteLength(p.innerText), 45)
    })

    it('correctly parses external one-byte UTF8 string', function () {
      var p = document.createElement('p')
      p.innerText = 'Jøhänñéß'
      var b = new Buffer(p.innerText)
      assert.equal(b.toString(), 'Jøhänñéß')
      assert.equal(Buffer.byteLength(p.innerText), 13)
    })

    it('does not crash when creating large Buffers', function () {
      new Buffer(new Array(4096).join(' '));
      new Buffer(new Array(4097).join(' '));
    })
  })

  describe('process.stdout', function () {
    it('should not throw exception', function () {
      process.stdout
    })

    it('should not throw exception when calling write()', function () {
      process.stdout.write('test')
    })

    xit('should have isTTY defined', function () {
      assert.equal(typeof process.stdout.isTTY, 'boolean')
    })
  })

  describe('process.version', function () {
    it('should not have -pre', function () {
      assert(!process.version.endsWith('-pre'))
    })
  })

  describe('vm.createContext', function () {
    it('should not crash', function () {
      require('vm').runInNewContext('')
    })
  })

  describe('require("electron")', function () {
    let window = null

    beforeEach(function () {
      if (window != null) {
        window.destroy()
      }
      window = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
    })

    afterEach(function () {
      if (window != null) {
        window.destroy()
      }
      window = null
    })

    it('always returns the internal electron module', function (done) {
      ipcMain.once('answer', function () {
        done()
      })
      window.loadURL('file://' + path.join(__dirname, 'fixtures', 'api', 'electron-module-app', 'index.html'))
    })
  })
})
