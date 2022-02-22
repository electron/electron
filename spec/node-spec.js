const ChildProcess = require('child_process');
const { expect } = require('chai');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { ipcRenderer } = require('electron');
const features = process._linkedBinding('electron_common_features');

const { emittedOnce } = require('./events-helpers');
const { ifdescribe, ifit } = require('./spec-helpers');

describe('node feature', () => {
  const fixtures = path.join(__dirname, 'fixtures');

  describe('child_process', () => {
    beforeEach(function () {
      if (!features.isRunAsNodeEnabled()) {
        this.skip();
      }
    });

    describe('child_process.fork', () => {
      it('works in current process', async () => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'ping.js'));
        const message = emittedOnce(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(msg).to.equal('message');
      });

      it('preserves args', async () => {
        const args = ['--expose_gc', '-test', '1'];
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'process_args.js'), args);
        const message = emittedOnce(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(args).to.deep.equal(msg.slice(2));
      });

      it('works in forked process', async () => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'fork_ping.js'));
        const message = emittedOnce(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(msg).to.equal('message');
      });

      it('works in forked process when options.env is specifed', async () => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'fork_ping.js'), [], {
          path: process.env.PATH
        });
        const message = emittedOnce(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(msg).to.equal('message');
      });

      it('has String::localeCompare working in script', async () => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'locale-compare.js'));
        const message = emittedOnce(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(msg).to.deep.equal([0, -1, 1]);
      });

      it('has setImmediate working in script', async () => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'set-immediate.js'));
        const message = emittedOnce(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(msg).to.equal('ok');
      });

      it('pipes stdio', async () => {
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'process-stdout.js'), { silent: true });
        let data = '';
        child.stdout.on('data', (chunk) => {
          data += String(chunk);
        });
        const [code] = await emittedOnce(child, 'close');
        expect(code).to.equal(0);
        expect(data).to.equal('pipes stdio');
      });

      it('works when sending a message to a process forked with the --eval argument', async () => {
        const source = "process.on('message', (message) => { process.send(message) })";
        const forked = ChildProcess.fork('--eval', [source]);
        const message = emittedOnce(forked, 'message');
        forked.send('hello');
        const [msg] = await message;
        expect(msg).to.equal('hello');
      });

      // TODO(jkleinsc) fix this test on Windows
      ifit(process.platform !== 'win32')('has the electron version in process.versions', async () => {
        const source = 'process.send(process.versions)';
        const forked = ChildProcess.fork('--eval', [source]);
        const [message] = await emittedOnce(forked, 'message');
        expect(message)
          .to.have.own.property('electron')
          .that.is.a('string')
          .and.matches(/^\d+\.\d+\.\d+(\S*)?$/);
      });
    });

    // TODO(jkleinsc) fix this test on Windows
    ifdescribe(process.platform !== 'win32')('child_process.spawn', () => {
      let child;

      afterEach(() => {
        if (child != null) child.kill();
      });

      it('supports spawning Electron as a node process via the ELECTRON_RUN_AS_NODE env var', async () => {
        child = ChildProcess.spawn(process.execPath, [path.join(__dirname, 'fixtures', 'module', 'run-as-node.js')], {
          env: {
            ELECTRON_RUN_AS_NODE: true
          }
        });

        let output = '';
        child.stdout.on('data', data => {
          output += data;
        });
        await emittedOnce(child.stdout, 'close');
        expect(JSON.parse(output)).to.deep.equal({
          stdoutType: 'pipe',
          processType: 'undefined',
          window: 'undefined'
        });
      });
    });

    describe('child_process.exec', () => {
      (process.platform === 'linux' ? it : it.skip)('allows executing a setuid binary from non-sandboxed renderer', () => {
        // Chrome uses prctl(2) to set the NO_NEW_PRIVILEGES flag on Linux (see
        // https://github.com/torvalds/linux/blob/40fde647cc/Documentation/userspace-api/no_new_privs.rst).
        // We disable this for unsandboxed processes, which the renderer tests
        // are running in. If this test fails with an error like 'effective uid
        // is not 0', then it's likely that our patch to prevent the flag from
        // being set has become ineffective.
        const stdout = ChildProcess.execSync('sudo --help');
        expect(stdout).to.not.be.empty();
      });
    });
  });

  describe('contexts', () => {
    describe('setTimeout in fs callback', () => {
      // TODO(jkleinsc) fix this test on Windows
      ifit(process.platform !== 'win32')('does not crash', (done) => {
        fs.readFile(__filename, () => {
          setTimeout(done, 0);
        });
      });
    });

    describe('error thrown in renderer process node context', () => {
      // TODO(jkleinsc) fix this test on Windows
      ifit(process.platform !== 'win32')('gets emitted as a process uncaughtException event', (done) => {
        const error = new Error('boo!');
        const listeners = process.listeners('uncaughtException');
        process.removeAllListeners('uncaughtException');
        process.on('uncaughtException', (thrown) => {
          try {
            expect(thrown).to.equal(error);
            done();
          } catch (e) {
            done(e);
          } finally {
            process.removeAllListeners('uncaughtException');
            listeners.forEach((listener) => process.on('uncaughtException', listener));
          }
        });
        fs.readFile(__filename, () => {
          throw error;
        });
      });
    });

    describe('URL handling in the renderer process', () => {
      it('can successfully handle WHATWG URLs constructed by Blink', () => {
        const url = new URL('file://' + path.resolve(fixtures, 'pages', 'base-page.html'));
        expect(() => {
          fs.createReadStream(url);
        }).to.not.throw();
      });
    });

    describe('error thrown in main process node context', () => {
      it('gets emitted as a process uncaughtException event', () => {
        const error = ipcRenderer.sendSync('handle-uncaught-exception', 'hello');
        expect(error).to.equal('hello');
      });
    });

    describe('promise rejection in main process node context', () => {
      it('gets emitted as a process unhandledRejection event', () => {
        const error = ipcRenderer.sendSync('handle-unhandled-rejection', 'hello');
        expect(error).to.equal('hello');
      });
    });

    describe('setTimeout called under blink env in renderer process', () => {
      it('can be scheduled in time', (done) => {
        setTimeout(done, 10);
      });

      it('works from the timers module', (done) => {
        require('timers').setTimeout(done, 10);
      });
    });

    describe('setInterval called under blink env in renderer process', () => {
      it('can be scheduled in time', (done) => {
        const id = setInterval(() => {
          clearInterval(id);
          done();
        }, 10);
      });

      it('can be scheduled in time from timers module', (done) => {
        const { setInterval, clearInterval } = require('timers');
        const id = setInterval(() => {
          clearInterval(id);
          done();
        }, 10);
      });
    });
  });

  describe('message loop', () => {
    describe('process.nextTick', () => {
      it('emits the callback', (done) => process.nextTick(done));

      it('works in nested calls', (done) => {
        process.nextTick(() => {
          process.nextTick(() => process.nextTick(done));
        });
      });
    });

    describe('setImmediate', () => {
      it('emits the callback', (done) => setImmediate(done));

      it('works in nested calls', (done) => {
        setImmediate(() => {
          setImmediate(() => setImmediate(done));
        });
      });
    });
  });

  describe('net.connect', () => {
    before(function () {
      if (!features.isRunAsNodeEnabled() || process.platform !== 'darwin') {
        this.skip();
      }
    });

    // TODO(jkleinsc) fix this flaky test on macOS
    ifit(process.platform !== 'darwin')('emit error when connect to a socket path without listeners', async () => {
      const socketPath = path.join(os.tmpdir(), 'atom-shell-test.sock');
      const script = path.join(fixtures, 'module', 'create_socket.js');
      const child = ChildProcess.fork(script, [socketPath]);
      const [code] = await emittedOnce(child, 'exit');
      expect(code).to.equal(0);
      const client = require('net').connect(socketPath);
      const [error] = await emittedOnce(client, 'error');
      expect(error.code).to.equal('ECONNREFUSED');
    });
  });

  describe('Buffer', () => {
    it('can be created from WebKit external string', () => {
      const p = document.createElement('p');
      p.innerText = '闲云潭影日悠悠，物换星移几度秋';
      const b = Buffer.from(p.innerText);
      expect(b.toString()).to.equal('闲云潭影日悠悠，物换星移几度秋');
      expect(Buffer.byteLength(p.innerText)).to.equal(45);
    });

    it('correctly parses external one-byte UTF8 string', () => {
      const p = document.createElement('p');
      p.innerText = 'Jøhänñéß';
      const b = Buffer.from(p.innerText);
      expect(b.toString()).to.equal('Jøhänñéß');
      expect(Buffer.byteLength(p.innerText)).to.equal(13);
    });

    it('does not crash when creating large Buffers', () => {
      let buffer = Buffer.from(new Array(4096).join(' '));
      expect(buffer.length).to.equal(4095);
      buffer = Buffer.from(new Array(4097).join(' '));
      expect(buffer.length).to.equal(4096);
    });

    it('does not crash for crypto operations', () => {
      const crypto = require('crypto');
      const data = 'lG9E+/g4JmRmedDAnihtBD4Dfaha/GFOjd+xUOQI05UtfVX3DjUXvrS98p7kZQwY3LNhdiFo7MY5rGft8yBuDhKuNNag9vRx/44IuClDhdQ=';
      const key = 'q90K9yBqhWZnAMCMTOJfPQ==';
      const cipherText = '{"error_code":114,"error_message":"Tham số không hợp lệ","data":null}';
      for (let i = 0; i < 10000; ++i) {
        const iv = Buffer.from('0'.repeat(32), 'hex');
        const input = Buffer.from(data, 'base64');
        const decipher = crypto.createDecipheriv('aes-128-cbc', Buffer.from(key, 'base64'), iv);
        const result = Buffer.concat([decipher.update(input), decipher.final()]).toString('utf8');
        expect(cipherText).to.equal(result);
      }
    });

    it('does not crash when using crypto.diffieHellman() constructors', () => {
      const crypto = require('crypto');

      crypto.createDiffieHellman('abc');
      crypto.createDiffieHellman('abc', 2);

      // Needed to test specific DiffieHellman ctors.

      // eslint-disable-next-line no-octal
      crypto.createDiffieHellman('abc', Buffer.from([02]));
      // eslint-disable-next-line no-octal
      crypto.createDiffieHellman('abc', '123');
    });

    it('does not crash when calling crypto.createPrivateKey() with an unsupported algorithm', () => {
      const crypto = require('crypto');

      const ed448 = {
        crv: 'Ed448',
        x: 'KYWcaDwgH77xdAwcbzOgvCVcGMy9I6prRQBhQTTdKXUcr-VquTz7Fd5adJO0wT2VHysF3bk3kBoA',
        d: 'UhC3-vN5vp_g9PnTknXZgfXUez7Xvw-OfuJ0pYkuwzpYkcTvacqoFkV_O05WMHpyXkzH9q2wzx5n',
        kty: 'OKP'
      };

      expect(() => {
        crypto.createPrivateKey({ key: ed448, format: 'jwk' });
      }).to.throw(/Invalid JWK data/);
    });
  });

  describe('process.stdout', () => {
    it('does not throw an exception when accessed', () => {
      expect(() => process.stdout).to.not.throw();
    });

    it('does not throw an exception when calling write()', () => {
      expect(() => {
        process.stdout.write('test');
      }).to.not.throw();
    });

    // TODO: figure out why process.stdout.isTTY is true on Darwin but not Linux/Win.
    ifit(process.platform !== 'darwin')('isTTY should be undefined in the renderer process', function () {
      expect(process.stdout.isTTY).to.be.undefined();
    });
  });

  describe('process.stdin', () => {
    it('does not throw an exception when accessed', () => {
      expect(() => process.stdin).to.not.throw();
    });

    it('returns null when read from', () => {
      expect(process.stdin.read()).to.be.null();
    });
  });

  describe('process.version', () => {
    it('should not have -pre', () => {
      expect(process.version.endsWith('-pre')).to.be.false();
    });
  });

  describe('vm.runInNewContext', () => {
    it('should not crash', () => {
      require('vm').runInNewContext('');
    });
  });

  describe('crypto', () => {
    it('should list the ripemd160 hash in getHashes', () => {
      expect(require('crypto').getHashes()).to.include('ripemd160');
    });

    it('should be able to create a ripemd160 hash and use it', () => {
      const hash = require('crypto').createHash('ripemd160');
      hash.update('electron-ripemd160');
      expect(hash.digest('hex')).to.equal('fa7fec13c624009ab126ebb99eda6525583395fe');
    });

    it('should list aes-{128,256}-cfb in getCiphers', () => {
      expect(require('crypto').getCiphers()).to.include.members(['aes-128-cfb', 'aes-256-cfb']);
    });

    it('should be able to create an aes-128-cfb cipher', () => {
      require('crypto').createCipheriv('aes-128-cfb', '0123456789abcdef', '0123456789abcdef');
    });

    it('should be able to create an aes-256-cfb cipher', () => {
      require('crypto').createCipheriv('aes-256-cfb', '0123456789abcdef0123456789abcdef', '0123456789abcdef');
    });

    it('should be able to create a bf-{cbc,cfb,ecb} ciphers', () => {
      require('crypto').createCipheriv('bf-cbc', Buffer.from('0123456789abcdef'), Buffer.from('01234567'));
      require('crypto').createCipheriv('bf-cfb', Buffer.from('0123456789abcdef'), Buffer.from('01234567'));
      require('crypto').createCipheriv('bf-ecb', Buffer.from('0123456789abcdef'), Buffer.from('01234567'));
    });

    it('should list des-ede-cbc in getCiphers', () => {
      expect(require('crypto').getCiphers()).to.include('des-ede-cbc');
    });

    it('should be able to create an des-ede-cbc cipher', () => {
      const key = Buffer.from('0123456789abcdeff1e0d3c2b5a49786', 'hex');
      const iv = Buffer.from('fedcba9876543210', 'hex');
      require('crypto').createCipheriv('des-ede-cbc', key, iv);
    });

    it('should not crash when getting an ECDH key', () => {
      const ecdh = require('crypto').createECDH('prime256v1');
      expect(ecdh.generateKeys()).to.be.an.instanceof(Buffer);
      expect(ecdh.getPrivateKey()).to.be.an.instanceof(Buffer);
    });

    it('should not crash when generating DH keys or fetching DH fields', () => {
      const dh = require('crypto').createDiffieHellman('modp15');
      expect(dh.generateKeys()).to.be.an.instanceof(Buffer);
      expect(dh.getPublicKey()).to.be.an.instanceof(Buffer);
      expect(dh.getPrivateKey()).to.be.an.instanceof(Buffer);
      expect(dh.getPrime()).to.be.an.instanceof(Buffer);
      expect(dh.getGenerator()).to.be.an.instanceof(Buffer);
    });

    it('should not crash when creating an ECDH cipher', () => {
      const crypto = require('crypto');
      const dh = crypto.createECDH('prime256v1');
      dh.generateKeys();
      dh.setPrivateKey(dh.getPrivateKey());
    });
  });

  it('includes the electron version in process.versions', () => {
    expect(process.versions)
      .to.have.own.property('electron')
      .that.is.a('string')
      .and.matches(/^\d+\.\d+\.\d+(\S*)?$/);
  });

  it('includes the chrome version in process.versions', () => {
    expect(process.versions)
      .to.have.own.property('chrome')
      .that.is.a('string')
      .and.matches(/^\d+\.\d+\.\d+\.\d+$/);
  });
});
