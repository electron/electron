import { expect } from 'chai'
import * as childProcess from 'child_process'
import * as path from 'path'
import * as util from 'util'
import { emittedOnce } from './events-helpers'
import { ifdescribe, ifit } from './spec-helpers'
import { webContents, WebContents } from 'electron'

const features = process.electronBinding('features')

describe('node feature', () => {
  const fixtures = path.join(__dirname, '..', 'spec', 'fixtures')
  describe('child_process', () => {
    describe('child_process.fork', () => {
      it('works in browser process', (done) => {
        const child = childProcess.fork(path.join(fixtures, 'module', 'ping.js'))
        child.on('message', (msg) => {
          expect(msg).to.equal('message')
          done()
        })
        child.send('message')
      })
    })
  })

  describe('contexts', () => {
    describe('setTimeout called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        setTimeout(done, 0)
      })

      it('can be promisified', (done) => {
        util.promisify(setTimeout)(0).then(done)
      })
    })

    describe('setInterval called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        let interval: any = null
        let clearing = false
        const clear = () => {
          if (interval === null || clearing) return

          // interval might trigger while clearing (remote is slow sometimes)
          clearing = true
          clearInterval(interval)
          clearing = false
          interval = null
          done()
        }
        interval = setInterval(clear, 10)
      })
    })
  })

  describe('NODE_OPTIONS', () => {
    let child: childProcess.ChildProcessWithoutNullStreams
    let exitPromise: Promise<any[]>

    afterEach(async () => {
      if (child && exitPromise) {
        const [code, signal] = await exitPromise
        expect(signal).to.equal(null)
        expect(code).to.equal(0)
      } else if (child) {
        child.kill()
      }
    })

    it('fails for options disallowed by Node.js itself', (done) => {
      const env = Object.assign({}, process.env, { NODE_OPTIONS: '--v8-options' })
      child = childProcess.spawn(process.execPath, { env })

      function cleanup () {
        child.stderr.removeListener('data', listener)
        child.stdout.removeListener('data', listener)
      }

      let output = ''
      function listener (data: Buffer) {
        output += data
        if (/electron: --v8-options is not allowed in NODE_OPTIONS/m.test(output)) {
          cleanup()
          done()
        }
      }
      child.stderr.on('data', listener)
      child.stdout.on('data', listener)
    })

    it('disallows crypto-related options', (done) => {
      const env = Object.assign({}, process.env, { NODE_OPTIONS: '--use-openssl-ca' })
      child = childProcess.spawn(process.execPath, ['--enable-logging'], { env })

      function cleanup () {
        child.stderr.removeListener('data', listener)
        child.stdout.removeListener('data', listener)
      }

      let output = ''
      function listener (data: Buffer) {
        output += data
        if (/The NODE_OPTION --use-openssl-ca is not supported in Electron/m.test(output)) {
          cleanup()
          done()
        }
      }
      child.stderr.on('data', listener)
      child.stdout.on('data', listener)
    })
  })

  ifdescribe(features.isRunAsNodeEnabled())('inspector', () => {
    let child: childProcess.ChildProcessWithoutNullStreams
    let exitPromise: Promise<any[]>

    afterEach(async () => {
      if (child && exitPromise) {
        const [code, signal] = await exitPromise
        expect(signal).to.equal(null)
        expect(code).to.equal(0)
      } else if (child) {
        child.kill()
      }
    })

    it('supports starting the v8 inspector with --inspect/--inspect-brk', (done) => {
      child = childProcess.spawn(process.execPath, ['--inspect-brk', path.join(fixtures, 'module', 'run-as-node.js')], {
        env: {
          ELECTRON_RUN_AS_NODE: 'true'
        }
      })

      let output = ''
      function cleanup () {
        child.stderr.removeListener('data', errorDataListener)
        child.stdout.removeListener('data', outDataHandler)
      }
      function errorDataListener (data: Buffer) {
        output += data
        if (/^Debugger listening on ws:/m.test(output)) {
          cleanup()
          done()
        }
      }
      function outDataHandler (data: Buffer) {
        cleanup()
        done(new Error(`Unexpected output: ${data.toString()}`))
      }
      child.stderr.on('data', errorDataListener)
      child.stdout.on('data', outDataHandler)
    })

    it('supports starting the v8 inspector with --inspect and a provided port', (done) => {
      child = childProcess.spawn(process.execPath, ['--inspect=17364', path.join(fixtures, 'module', 'run-as-node.js')], {
        env: {
          ELECTRON_RUN_AS_NODE: 'true'
        }
      })
      exitPromise = emittedOnce(child, 'exit')

      let output = ''
      function cleanup () {
        child.stderr.removeListener('data', errorDataListener)
        child.stdout.removeListener('data', outDataHandler)
      }
      function errorDataListener (data: Buffer) {
        output += data
        if (/^Debugger listening on ws:/m.test(output)) {
          expect(output.trim()).to.contain(':17364', 'should be listening on port 17364')
          cleanup()
          done()
        }
      }
      function outDataHandler (data: Buffer) {
        cleanup()
        done(new Error(`Unexpected output: ${data.toString()}`))
      }
      child.stderr.on('data', errorDataListener)
      child.stdout.on('data', outDataHandler)
    })

    it('does not start the v8 inspector when --inspect is after a -- argument', (done) => {
      child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'noop.js'), '--', '--inspect'])
      exitPromise = emittedOnce(child, 'exit')

      let output = ''
      function dataListener (data: Buffer) {
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

    // IPC Electron child process not supported on Windows
    ifit(process.platform !== 'win32')('does does not crash when quitting with the inspector connected', function (done) {
      child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'delay-exit'), '--inspect=0'], {
        stdio: ['ipc']
      }) as childProcess.ChildProcessWithoutNullStreams
      exitPromise = emittedOnce(child, 'exit')

      let output = ''
      function dataListener (data: Buffer) {
        output += data

        if (output.trim().indexOf('Debugger listening on ws://') > -1 && output.indexOf('\n') > -1) {
          const socketMatch = output.trim().match(/(ws:\/\/.+:[0-9]+\/.+?)\n/gm)
          if (socketMatch && socketMatch[0]) {
            child.stderr.removeListener('data', dataListener)
            child.stdout.removeListener('data', dataListener)

            const w = (webContents as any).create({}) as WebContents
            w.loadURL('about:blank')
              .then(() => w.executeJavaScript(`new Promise(resolve => {
                const connection = new WebSocket(${JSON.stringify(socketMatch[0])})
                connection.onopen = () => {
                  resolve()
                  connection.close()
                }
              })`))
              .then(() => {
                (w as any).destroy()
                child.send('plz-quit')
                done()
              })
          }
        }
      }
      child.stderr.on('data', dataListener)
      child.stdout.on('data', dataListener)
    })

    it('supports js binding', (done) => {
      child = childProcess.spawn(process.execPath, ['--inspect', path.join(fixtures, 'module', 'inspector-binding.js')], {
        env: {
          ELECTRON_RUN_AS_NODE: 'true'
        },
        stdio: ['ipc']
      }) as childProcess.ChildProcessWithoutNullStreams
      exitPromise = emittedOnce(child, 'exit')

      child.on('message', ({ cmd, debuggerEnabled, success }) => {
        if (cmd === 'assert') {
          expect(debuggerEnabled).to.be.true()
          expect(success).to.be.true()
          done()
        }
      })
    })
  })

  it('can find a module using a package.json main field', () => {
    const result = childProcess.spawnSync(process.execPath, [path.resolve(fixtures, 'api', 'electron-main-module', 'app.asar')])
    expect(result.status).to.equal(0)
  })
})
