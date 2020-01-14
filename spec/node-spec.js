const ChildProcess = require('child_process')
const { expect } = require('chai')
const fs = require('fs')
const path = require('path')
const os = require('os')
const { ipcRenderer } = require('electron')
const features = process.electronBinding('features')

const { emittedOnce } = require('./events-helpers')
const { ifit } = require('./spec-helpers')

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

    describe('child_process.exec', () => {
      (process.platform === 'linux' ? it : it.skip)('allows executing a setuid binary from non-sandboxed renderer', () => {
        // Chrome uses prctl(2) to set the NO_NEW_PRIVILEGES flag on Linux (see
        // https://github.com/torvalds/linux/blob/40fde647cc/Documentation/userspace-api/no_new_privs.rst).
        // We disable this for unsandboxed processes, which the renderer tests
        // are running in. If this test fails with an error like 'effective uid
        // is not 0', then it's likely that our patch to prevent the flag from
        // being set has become ineffective.
        const stdout = ChildProcess.execSync('sudo --help')
        expect(stdout).to.not.be.empty()
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

    describe('setTimeout called under blink env in renderer process', () => {
      it('can be scheduled in time', (done) => {
        setTimeout(done, 10)
      })

      it('works from the timers module', (done) => {
        require('timers').setTimeout(done, 10)
      })
    })

    describe('setInterval called under blink env in renderer process', () => {
      it('can be scheduled in time', (done) => {
        const id = setInterval(() => {
          clearInterval(id)
          done()
        }, 10)
      })

      it('can be scheduled in time from timers module', (done) => {
        const { setInterval, clearInterval } = require('timers')
        const id = setInterval(() => {
          clearInterval(id)
          done()
        }, 10)
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

    // TODO: figure out why process.stdout.isTTY is true on Darwin but not Linux/Win.
    ifit(process.platform !== 'darwin')('isTTY should be undefined in the renderer process', function () {
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

    it('should not crash when getting an ECDH key', () => {
      const ecdh = require('crypto').createECDH('prime256v1')
      expect(ecdh.generateKeys()).to.be.an.instanceof(Buffer)
      expect(ecdh.getPrivateKey()).to.be.an.instanceof(Buffer)
    })

    it('should not crash when generating DH keys or fetching DH fields', () => {
      const dh = require('crypto').createDiffieHellman('modp15')
      expect(dh.generateKeys()).to.be.an.instanceof(Buffer)
      expect(dh.getPublicKey()).to.be.an.instanceof(Buffer)
      expect(dh.getPrivateKey()).to.be.an.instanceof(Buffer)
      expect(dh.getPrime()).to.be.an.instanceof(Buffer)
      expect(dh.getGenerator()).to.be.an.instanceof(Buffer)
    })

    it('should not crash when creating an ECDH cipher', () => {
      const crypto = require('crypto')
      const dh = crypto.createECDH('prime256v1')
      dh.generateKeys()
      dh.setPrivateKey(dh.getPrivateKey())
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
