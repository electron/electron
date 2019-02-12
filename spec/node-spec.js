const ChildProcess = require('child_process')
const chai = require('chai')
const { expect } = chai
const dirtyChai = require('dirty-chai')
const fs = require('fs')
const path = require('path')
const os = require('os')
const { ipcRenderer, remote } = require('electron')
const features = process.atomBinding('features')

const isCI = remote.getGlobal('isCi')
chai.use(dirtyChai)

describe('node feature', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('child_process', () => {
    beforeEach(function () {
      if (!features.isRunAsNodeEnabled()) {
        this.skip()
      }
    })

    describe('child_process.fork', () => {
      it('works in current process', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', msg => {
          expect(msg).to.equal('message')
          done()
        })
        child.send('message')
      })

      it('preserves args', (done) => {
        const args = ['--expose_gc', '-test', '1']
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'process_args.js'), args)
        child.on('message', (msg) => {
          expect(args).to.deep.equal(msg.slice(2))
          done()
        })
        child.send('message')
      })

      it('works in forked process', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'fork_ping.js'))
        child.on('message', (msg) => {
          expect(msg).to.equal('message')
          done()
        })
        child.send('message')
      })

      it('works in forked process when options.env is specifed', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'fork_ping.js'), [], {
          path: process.env['PATH']
        })
        child.on('message', (msg) => {
          expect(msg).to.equal('message')
          done()
        })
        child.send('message')
      })

      it('works in browser process', (done) => {
        const fork = remote.require('child_process').fork
        const child = fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', (msg) => {
          expect(msg).to.equal('message')
          done()
        })
        child.send('message')
      })

      it('has String::localeCompare working in script', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'locale-compare.js'))
        child.on('message', (msg) => {
          expect(msg).to.deep.equal([0, -1, 1])
          done()
        })
        child.send('message')
      })

      it('has setImmediate working in script', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'set-immediate.js'))
        child.on('message', (msg) => {
          expect(msg).to.equal('ok')
          done()
        })
        child.send('message')
      })

      it('pipes stdio', (done) => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'process-stdout.js'), { silent: true })
        let data = ''
        child.stdout.on('data', (chunk) => {
          data += String(chunk)
        })
        child.on('close', (code) => {
          expect(code).to.equal(0)
          expect(data).to.equal('pipes stdio')
          done()
        })
      })

      it('works when sending a message to a process forked with the --eval argument', (done) => {
        const source = "process.on('message', (message) => { process.send(message) })"
        const forked = ChildProcess.fork('--eval', [source])
        forked.once('message', (message) => {
          expect(message).to.equal('hello')
          done()
        })
        forked.send('hello')
      })

      it('has the electron version in process.versions', (done) => {
        const source = 'process.send(process.versions)'
        const forked = ChildProcess.fork('--eval', [source])
        forked.on('message', (message) => {
          expect(message)
            .to.have.own.property('electron')
            .that.is.a('string')
            .and.matches(/^\d+\.\d+\.\d+(\S*)?$/)
          done()
        })
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
        child.stdout.on('data', data => {
          output += data
        })
        child.stdout.on('close', () => {
          expect(JSON.parse(output)).to.deep.equal({
            processLog: process.platform === 'win32' ? 'function' : 'undefined',
            processType: 'undefined',
            window: 'undefined'
          })
          done()
        })
      })
    })
  })

  describe('contexts', () => {
    describe('setTimeout in fs callback', () => {
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
          try {
            expect(thrown).to.equal(error)
            done()
          } catch (e) {
            done(e)
          } finally {
            process.removeAllListeners('uncaughtException')
            listeners.forEach((listener) => process.on('uncaughtException', listener))
          }
        })
        fs.readFile(__filename, () => {
          throw error
        })
      })
    })

    describe('error thrown in main process node context', () => {
      it('gets emitted as a process uncaughtException event', () => {
        const error = ipcRenderer.sendSync('handle-uncaught-exception', 'hello')
        expect(error).to.equal('hello')
      })
    })

    describe('promise rejection in main process node context', () => {
      it('gets emitted as a process unhandledRejection event', () => {
        const error = ipcRenderer.sendSync('handle-unhandled-rejection', 'hello')
        expect(error).to.equal('hello')
      })
    })

    describe('setTimeout called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        remote.getGlobal('setTimeout')(done, 0)
      })

      it('can be promisified', (done) => {
        remote.getGlobal('setTimeoutPromisified')(0).then(done)
      })
    })

    describe('setInterval called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        let interval = null
        let clearing = false
        const clear = () => {
          if (interval === null || clearing) return

          // interval might trigger while clearing (remote is slow sometimes)
          clearing = true
          remote.getGlobal('clearInterval')(interval)
          clearing = false
          interval = null
          done()
        }
        interval = remote.getGlobal('setInterval')(clear, 10)
      })
    })
  })

  describe('inspector', () => {
    let child = null

    beforeEach(function () {
      if (!features.isRunAsNodeEnabled()) {
        this.skip()
      }
    })

    afterEach(() => {
      if (child !== null) child.kill()
    })

    it('supports starting the v8 inspector with --inspect/--inspect-brk', (done) => {
      child = ChildProcess.spawn(process.execPath, ['--inspect-brk', path.join(__dirname, 'fixtures', 'module', 'run-as-node.js')], {
        env: {
          ELECTRON_RUN_AS_NODE: true
        }
      })

      let output = ''
      function cleanup () {
        child.stderr.removeListener('data', errorDataListener)
        child.stdout.removeListener('data', outDataHandler)
      }
      function errorDataListener (data) {
        output += data
        if (output.trim().startsWith('Debugger listening on ws://')) {
          cleanup()
          done()
        }
      }
      function outDataHandler (data) {
        cleanup()
        done(new Error(`Unexpected output: ${data.toString()}`))
      }
      child.stderr.on('data', errorDataListener)
      child.stdout.on('data', outDataHandler)
    })

    it('does not start the v8 inspector when --inspect is after a -- argument', (done) => {
      child = ChildProcess.spawn(remote.process.execPath, [path.join(__dirname, 'fixtures', 'module', 'noop.js'), '--', '--inspect'])

      let output = ''
      function dataListener (data) {
        output += data
      }
      child.stderr.on('data', dataListener)
      child.stdout.on('data', dataListener)
      child.on('exit', () => {
        if (output.trim().startsWith('Debugger listening on ws://')) {
          done(new Error('Inspector was started when it should not have been'))
        } else {
          done()
        }
      })
    })

    it('supports js binding', (done) => {
      child = ChildProcess.spawn(process.execPath, ['--inspect', path.join(__dirname, 'fixtures', 'module', 'inspector-binding.js')], {
        env: {
          ELECTRON_RUN_AS_NODE: true
        },
        stdio: ['ipc']
      })

      child.on('message', ({ cmd, debuggerEnabled, success }) => {
        if (cmd === 'assert') {
          expect(debuggerEnabled).to.be.true()
          expect(success).to.be.true()
          done()
        }
      })
    })
  })

  describe('message loop', () => {
    describe('process.nextTick', () => {
      it('emits the callback', (done) => process.nextTick(done))

      it('works in nested calls', (done) => {
        process.nextTick(() => {
          process.nextTick(() => process.nextTick(done))
        })
      })
    })

    describe('setImmediate', () => {
      it('emits the callback', (done) => setImmediate(done))

      it('works in nested calls', (done) => {
        setImmediate(() => {
          setImmediate(() => setImmediate(done))
        })
      })
    })
  })

  describe('net.connect', () => {
    before(function () {
      if (!features.isRunAsNodeEnabled() || process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('emit error when connect to a socket path without listeners', (done) => {
      const socketPath = path.join(os.tmpdir(), 'atom-shell-test.sock')
      const script = path.join(fixtures, 'module', 'create_socket.js')
      const child = ChildProcess.fork(script, [socketPath])
      child.on('exit', (code) => {
        expect(code).to.equal(0)
        const client = require('net').connect(socketPath)
        client.on('error', (error) => {
          expect(error.code).to.equal('ECONNREFUSED')
          done()
        })
      })
    })
  })

  describe('Buffer', () => {
    it('can be created from WebKit external string', () => {
      const p = document.createElement('p')
      p.innerText = '闲云潭影日悠悠，物换星移几度秋'
      const b = Buffer.from(p.innerText)
      expect(b.toString()).to.equal('闲云潭影日悠悠，物换星移几度秋')
      expect(Buffer.byteLength(p.innerText)).to.equal(45)
    })

    it('correctly parses external one-byte UTF8 string', () => {
      const p = document.createElement('p')
      p.innerText = 'Jøhänñéß'
      const b = Buffer.from(p.innerText)
      expect(b.toString()).to.equal('Jøhänñéß')
      expect(Buffer.byteLength(p.innerText)).to.equal(13)
    })

    it('does not crash when creating large Buffers', () => {
      let buffer = Buffer.from(new Array(4096).join(' '))
      expect(buffer.length).to.equal(4095)
      buffer = Buffer.from(new Array(4097).join(' '))
      expect(buffer.length).to.equal(4096)
    })

    it('does not crash for crypto operations', () => {
      const crypto = require('crypto')
      const data = 'lG9E+/g4JmRmedDAnihtBD4Dfaha/GFOjd+xUOQI05UtfVX3DjUXvrS98p7kZQwY3LNhdiFo7MY5rGft8yBuDhKuNNag9vRx/44IuClDhdQ='
      const key = 'q90K9yBqhWZnAMCMTOJfPQ=='
      const cipherText = '{"error_code":114,"error_message":"Tham số không hợp lệ","data":null}'
      for (let i = 0; i < 10000; ++i) {
        const iv = Buffer.from('0'.repeat(32), 'hex')
        const input = Buffer.from(data, 'base64')
        const decipher = crypto.createDecipheriv('aes-128-cbc', Buffer.from(key, 'base64'), iv)
        const result = Buffer.concat([decipher.update(input), decipher.final()]).toString('utf8')
        expect(cipherText).to.equal(result)
      }
    })
  })

  describe('process.stdout', () => {
    it('does not throw an exception when accessed', () => {
      expect(() => process.stdout).to.not.throw()
    })

    it('does not throw an exception when calling write()', () => {
      expect(() => {
        process.stdout.write('test')
      }).to.not.throw()
    })

    it('should have isTTY defined on Mac and Linux', function () {
      if (isCI || process.platform === 'win32') {
        this.skip()
        return
      }

      expect(process.stdout.isTTY).to.be.a('boolean')
    })

    it('should have isTTY undefined on Windows', function () {
      if (isCI || process.platform !== 'win32') {
        this.skip()
        return
      }

      expect(process.stdout.isTTY).to.be.undefined()
    })
  })

  describe('process.stdin', () => {
    it('does not throw an exception when accessed', () => {
      expect(() => process.stdin).to.not.throw()
    })

    it('returns null when read from', () => {
      expect(process.stdin.read()).to.be.null()
    })
  })

  describe('process.version', () => {
    it('should not have -pre', () => {
      expect(process.version.endsWith('-pre')).to.be.false()
    })
  })

  describe('vm.runInNewContext', () => {
    it('should not crash', () => {
      require('vm').runInNewContext('')
    })
  })

  describe('crypto', () => {
    it('should list the ripemd160 hash in getHashes', () => {
      expect(require('crypto').getHashes()).to.include('ripemd160')
    })

    it('should be able to create a ripemd160 hash and use it', () => {
      const hash = require('crypto').createHash('ripemd160')
      hash.update('electron-ripemd160')
      expect(hash.digest('hex')).to.equal('fa7fec13c624009ab126ebb99eda6525583395fe')
    })

    it('should list aes-{128,256}-cfb in getCiphers', () => {
      expect(require('crypto').getCiphers()).to.include.members(['aes-128-cfb', 'aes-256-cfb'])
    })

    it('should be able to create an aes-128-cfb cipher', () => {
      require('crypto').createCipheriv('aes-128-cfb', '0123456789abcdef', '0123456789abcdef')
    })

    it('should be able to create an aes-256-cfb cipher', () => {
      require('crypto').createCipheriv('aes-256-cfb', '0123456789abcdef0123456789abcdef', '0123456789abcdef')
    })

    it('should list des-ede-cbc in getCiphers', () => {
      expect(require('crypto').getCiphers()).to.include('des-ede-cbc')
    })

    it('should be able to create an des-ede-cbc cipher', () => {
      const key = Buffer.from('0123456789abcdeff1e0d3c2b5a49786', 'hex')
      const iv = Buffer.from('fedcba9876543210', 'hex')
      require('crypto').createCipheriv('des-ede-cbc', key, iv)
    })
  })

  it('includes the electron version in process.versions', () => {
    expect(process.versions)
      .to.have.own.property('electron')
      .that.is.a('string')
      .and.matches(/^\d+\.\d+\.\d+(\S*)?$/)
  })

  it('includes the chrome version in process.versions', () => {
    expect(process.versions)
      .to.have.own.property('chrome')
      .that.is.a('string')
      .and.matches(/^\d+\.\d+\.\d+\.\d+$/)
  })
})
