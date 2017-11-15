const assert = require('assert')
const ChildProcess = require('child_process')
const fs = require('fs')
const path = require('path')
const os = require('os')
const {ipcRenderer, remote} = require('electron')

const isCI = remote.getGlobal('isCi')

describe('node feature', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('child_process', () => {
    describe('child_process.fork', () => {
      it('works in current process', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', (msg) => {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('preserves args', (done) => {
        const args = ['--expose_gc', '-test', '1']
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'process_args.js'), args)
        child.on('message', (msg) => {
          assert.deepEqual(args, msg.slice(2))
          done()
        })
        child.send('message')
      })

      it('works in forked process', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'fork_ping.js'))
        child.on('message', (msg) => {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('works in forked process when options.env is specifed', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'fork_ping.js'), [], {
          path: process.env['PATH']
        })
        child.on('message', (msg) => {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('works in browser process', (done) => {
        const fork = remote.require('child_process').fork
        const child = fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', (msg) => {
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('has String::localeCompare working in script', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'locale-compare.js'))
        child.on('message', (msg) => {
          assert.deepEqual(msg, [0, -1, 1])
          done()
        })
        child.send('message')
      })

      it('has setImmediate working in script', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'set-immediate.js'))
        child.on('message', (msg) => {
          assert.equal(msg, 'ok')
          done()
        })
        child.send('message')
      })

      it('pipes stdio', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'process-stdout.js'), {silent: true})
        let data = ''
        child.stdout.on('data', (chunk) => {
          data += String(chunk)
        })
        child.on('close', (code) => {
          assert.equal(code, 0)
          assert.equal(data, 'pipes stdio')
          done()
        })
      })

      it('works when sending a message to a process forked with the --eval argument', (done) => {
        const source = "process.on('message', (message) => { process.send(message) })"
        const forked = ChildProcess.fork('--eval', [source])
        forked.once('message', (message) => {
          assert.equal(message, 'hello')
          done()
        })
        forked.send('hello')
      })
    })

    describe('child_process.spawn', () => {
      let child

      afterEach(() => {
        if (child != null) child.kill()
      })

      it('supports spawning Electron as a node process via the ELECTRON_RUN_AS_NODE env var', (done) => {
        child = ChildProcess.spawn(process.execPath, [path.join(__dirname, 'fixtures', 'module', 'run-as-node.js')], {
          env: {
            ELECTRON_RUN_AS_NODE: true
          }
        })

        let output = ''
        child.stdout.on('data', (data) => {
          output += data
        })
        child.stdout.on('close', () => {
          assert.deepEqual(JSON.parse(output), {
            processLog: process.platform === 'win32' ? 'function' : 'undefined',
            processType: 'undefined',
            window: 'undefined'
          })
          done()
        })
      })

      it('supports starting the v8 inspector with --inspect/--inspect-brk', (done) => {
        child = ChildProcess.spawn(process.execPath, ['--inspect-brk', path.join(__dirname, 'fixtures', 'module', 'run-as-node.js')], {
          env: {
            ELECTRON_RUN_AS_NODE: true
          }
        })

        let output = ''
        child.stderr.on('data', (data) => {
          output += data
          if (output.trim().startsWith('Debugger listening on ws://')) done()
        })

        child.stdout.on('data', (data) => {
          done(new Error(`Unexpected output: ${data.toString()}`))
        })
      })
    })
  })

  describe('contexts', () => {
    describe('setTimeout in fs callback', () => {
      if (process.env.TRAVIS === 'true') return

      it('does not crash', (done) => {
        fs.readFile(__filename, () => {
          setTimeout(done, 0)
        })
      })
    })

    describe('error thrown in renderer process node context', () => {
      it('gets emitted as a process uncaughtException event', (done) => {
        const error = new Error('boo!')
        const listeners = process.listeners('uncaughtException')
        process.removeAllListeners('uncaughtException')
        process.on('uncaughtException', (thrown) => {
          assert.strictEqual(thrown, error)
          process.removeAllListeners('uncaughtException')
          listeners.forEach((listener) => {
            process.on('uncaughtException', listener)
          })
          done()
        })
        fs.readFile(__filename, () => {
          throw error
        })
      })
    })

    describe('error thrown in main process node context', () => {
      it('gets emitted as a process uncaughtException event', () => {
        const error = ipcRenderer.sendSync('handle-uncaught-exception', 'hello')
        assert.equal(error, 'hello')
      })
    })

    describe('promise rejection in main process node context', () => {
      it('gets emitted as a process unhandledRejection event', () => {
        const error = ipcRenderer.sendSync('handle-unhandled-rejection', 'hello')
        assert.equal(error, 'hello')
      })
    })

    describe('setTimeout called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        remote.getGlobal('setTimeout')(done, 0)
      })
    })

    describe('setInterval called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        let clear
        let interval
        clear = () => {
          remote.getGlobal('clearInterval')(interval)
          done()
        }
        interval = remote.getGlobal('setInterval')(clear, 10)
      })
    })
  })

  describe('message loop', () => {
    describe('process.nextTick', () => {
      it('emits the callback', (done) => {
        process.nextTick(done)
      })

      it('works in nested calls', (done) => {
        process.nextTick(() => {
          process.nextTick(() => {
            process.nextTick(done)
          })
        })
      })
    })

    describe('setImmediate', () => {
      it('emits the callback', (done) => {
        setImmediate(done)
      })

      it('works in nested calls', (done) => {
        setImmediate(() => {
          setImmediate(() => {
            setImmediate(done)
          })
        })
      })
    })
  })

  describe('net.connect', () => {
    if (process.platform !== 'darwin') return

    it('emit error when connect to a socket path without listeners', (done) => {
      const socketPath = path.join(os.tmpdir(), 'atom-shell-test.sock')
      const script = path.join(fixtures, 'module', 'create_socket.js')
      const child = ChildProcess.fork(script, [socketPath])
      child.on('exit', (code) => {
        assert.equal(code, 0)
        const client = require('net').connect(socketPath)
        client.on('error', (error) => {
          assert.equal(error.code, 'ECONNREFUSED')
          done()
        })
      })
    })
  })

  describe('Buffer', () => {
    it('can be created from WebKit external string', () => {
      const p = document.createElement('p')
      p.innerText = '闲云潭影日悠悠，物换星移几度秋'
      const b = new Buffer(p.innerText)
      assert.equal(b.toString(), '闲云潭影日悠悠，物换星移几度秋')
      assert.equal(Buffer.byteLength(p.innerText), 45)
    })

    it('correctly parses external one-byte UTF8 string', () => {
      const p = document.createElement('p')
      p.innerText = 'Jøhänñéß'
      const b = new Buffer(p.innerText)
      assert.equal(b.toString(), 'Jøhänñéß')
      assert.equal(Buffer.byteLength(p.innerText), 13)
    })

    it('does not crash when creating large Buffers', () => {
      let buffer = new Buffer(new Array(4096).join(' '))
      assert.equal(buffer.length, 4095)
      buffer = new Buffer(new Array(4097).join(' '))
      assert.equal(buffer.length, 4096)
    })
  })

  describe('process.stdout', () => {
    it('does not throw an exception when accessed', () => {
      assert.doesNotThrow(() => {
        process.stdout
      })
    })

    it('does not throw an exception when calling write()', () => {
      assert.doesNotThrow(() => {
        process.stdout.write('test')
      })
    })

    it('should have isTTY defined on Mac and Linux', () => {
      if (isCI) return

      if (process.platform === 'win32') {
        assert.equal(process.stdout.isTTY, undefined)
      } else {
        assert.equal(typeof process.stdout.isTTY, 'boolean')
      }
    })
  })

  describe('process.stdin', () => {
    it('does not throw an exception when accessed', () => {
      assert.doesNotThrow(() => {
        process.stdin
      })
    })

    it('returns null when read from', () => {
      assert.equal(process.stdin.read(), null)
    })
  })

  describe('process.version', () => {
    it('should not have -pre', () => {
      assert(!process.version.endsWith('-pre'))
    })
  })

  describe('vm.createContext', () => {
    it('should not crash', () => {
      require('vm').runInNewContext('')
    })
  })

  it('includes the electron version in process.versions', () => {
    assert(/^\d+\.\d+\.\d+(\S*)?$/.test(process.versions.electron))
  })

  it('includes the chrome version in process.versions', () => {
    assert(/^\d+\.\d+\.\d+\.\d+$/.test(process.versions.chrome))
  })
})
