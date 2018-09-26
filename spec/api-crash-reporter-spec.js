const assert = require('assert')
const childProcess = require('child_process')
const { expect } = require('chai')
const fs = require('fs')
const http = require('http')
const multiparty = require('multiparty')
const path = require('path')
const temp = require('temp').track()
const url = require('url')
const { closeWindow } = require('./window-helpers')

const { remote } = require('electron')
const { app, BrowserWindow, crashReporter } = remote.require('electron')

describe('crashReporter module', () => {
  if (process.mas || process.env.DISABLE_CRASH_REPORTER_TESTS) return

  // TODO(alexeykuzmin): [Ch66] Fails. Fix it and enable back.
  if (process.platform === 'linux') return

  let originalTempDirectory = null
  let tempDirectory = null

  before(() => {
    tempDirectory = temp.mkdirSync('electronCrashReporterSpec-')
    originalTempDirectory = app.getPath('temp')
    app.setPath('temp', tempDirectory)
  })

  after(() => {
    app.setPath('temp', originalTempDirectory)
  })

  const fixtures = path.resolve(__dirname, 'fixtures')
  const generateSpecs = (description, browserWindowOpts) => {
    describe(description, () => {
      let w = null
      let stopServer = null

      beforeEach(() => {
        stopServer = null
        w = new BrowserWindow(Object.assign({ show: false }, browserWindowOpts))
      })

      afterEach(() => closeWindow(w).then(() => { w = null }))

      afterEach(() => {
        stopCrashService()
      })

      afterEach((done) => {
        if (stopServer != null) {
          stopServer(done)
        } else {
          done()
        }
      })

      it('should send minidump when renderer crashes', function (done) {
        // TODO(alexeykuzmin): Skip the test instead of marking it as passed.
        if (process.env.APPVEYOR === 'True') return done()

        this.timeout(180000)

        stopServer = startServer({
          callback (port) {
            w.loadFile(path.join(fixtures, 'api', 'crash.html'), { query: { port } })
          },
          processType: 'renderer',
          done: done
        })
      })

      it('should send minidump when node processes crash', function (done) {
        // TODO(alexeykuzmin): Skip the test instead of marking it as passed.
        if (process.env.APPVEYOR === 'True') return done()

        this.timeout(180000)

        stopServer = startServer({
          callback (port) {
            const crashesDir = path.join(app.getPath('temp'), `${process.platform === 'win32' ? 'Zombies' : app.getName()} Crashes`)
            const version = app.getVersion()
            const crashPath = path.join(fixtures, 'module', 'crash.js')

            if (process.platform === 'win32') {
              const crashServiceProcess = childProcess.spawn(process.execPath, [
                `--reporter-url=http://127.0.0.1:${port}`,
                '--application-name=Zombies',
                `--crashes-directory=${crashesDir}`
              ], {
                env: {
                  ELECTRON_INTERNAL_CRASH_SERVICE: 1
                },
                detached: true
              })
              remote.process.crashServicePid = crashServiceProcess.pid
            }

            childProcess.fork(crashPath, [port, version, crashesDir], { silent: true })
          },
          processType: 'browser',
          done: done
        })
      })

      it('should not send minidump if uploadToServer is false', function (done) {
        this.timeout(180000)

        let dumpFile
        let crashesDir = crashReporter.getCrashesDirectory()
        const existingDumpFiles = new Set()
        if (process.platform === 'darwin') {
          // crashpad puts the dump files in the "completed" subdirectory
          crashesDir = path.join(crashesDir, 'completed')
          crashReporter.setUploadToServer(false)
        }
        const testDone = (uploaded) => {
          if (uploaded) return done(new Error('Uploaded crash report'))
          if (process.platform === 'darwin') crashReporter.setUploadToServer(true)
          assert(fs.existsSync(dumpFile))
          done()
        }

        let pollInterval
        const pollDumpFile = () => {
          fs.readdir(crashesDir, (err, files) => {
            if (err) return
            const dumps = files.filter((file) => /\.dmp$/.test(file) && !existingDumpFiles.has(file))
            if (!dumps.length) return

            assert.strictEqual(1, dumps.length)
            dumpFile = path.join(crashesDir, dumps[0])
            clearInterval(pollInterval)
            // dump file should not be deleted when not uploading, so we wait
            // 1s and assert it still exists in `testDone`
            setTimeout(testDone, 1000)
          })
        }

        remote.ipcMain.once('list-existing-dumps', (event) => {
          fs.readdir(crashesDir, (err, files) => {
            if (!err) {
              for (const file of files) {
                if (/\.dmp$/.test(file)) {
                  existingDumpFiles.add(file)
                }
              }
            }
            event.returnValue = null // allow the renderer to crash
            pollInterval = setInterval(pollDumpFile, 100)
          })
        })

        stopServer = startServer({
          callback (port) {
            const crashUrl = url.format({
              protocol: 'file',
              pathname: path.join(fixtures, 'api', 'crash.html'),
              search: `?port=${port}&skipUpload=1`
            })
            w.loadURL(crashUrl)
          },
          processType: 'renderer',
          done: testDone.bind(null, true)
        })
      })

      it('should send minidump with updated extra parameters', function (done) {
        // TODO(alexeykuzmin): Skip the test instead of marking it as passed.
        if (process.env.APPVEYOR === 'True') return done()

        this.timeout(180000)

        stopServer = startServer({
          callback (port) {
            const crashUrl = url.format({
              protocol: 'file',
              pathname: path.join(fixtures, 'api', 'crash-restart.html'),
              search: `?port=${port}`
            })
            w.loadURL(crashUrl)
          },
          processType: 'renderer',
          done: done
        })
      })
    })
  }

  generateSpecs('without sandbox', {})
  generateSpecs('with sandbox', {
    webPreferences: {
      sandbox: true,
      preload: path.join(fixtures, 'module', 'preload-sandbox.js')
    }
  })

  describe('getProductName', () => {
    it('returns the product name if one is specified', () => {
      const name = crashReporter.getProductName()
      const expectedName = (process.platform === 'darwin') ? 'Electron Test' : 'Zombies'
      assert.strictEqual(name, expectedName)
    })
  })

  describe('start(options)', () => {
    it('requires that the companyName and submitURL options be specified', () => {
      assert.throws(() => {
        crashReporter.start({ companyName: 'Missing submitURL' })
      }, /submitURL is a required option to crashReporter\.start/)
      assert.throws(() => {
        crashReporter.start({ submitURL: 'Missing companyName' })
      }, /companyName is a required option to crashReporter\.start/)
    })
    it('can be called multiple times', () => {
      assert.doesNotThrow(() => {
        crashReporter.start({
          companyName: 'Umbrella Corporation',
          submitURL: 'http://127.0.0.1/crashes'
        })

        crashReporter.start({
          companyName: 'Umbrella Corporation 2',
          submitURL: 'http://127.0.0.1/more-crashes'
        })
      })
    })
  })

  describe('getCrashesDirectory', () => {
    it('correctly returns the directory', () => {
      const crashesDir = crashReporter.getCrashesDirectory()
      let dir
      if (process.platform === 'win32') {
        dir = `${app.getPath('temp')}/Zombies Crashes`
      } else {
        dir = `${app.getPath('temp')}/Electron Test Crashes`
      }
      assert.strictEqual(crashesDir, dir)
    })
  })

  describe('getUploadedReports', () => {
    it('returns an array of reports', () => {
      const reports = crashReporter.getUploadedReports()
      assert(typeof reports === 'object')
    })
  })

  // TODO(alexeykuzmin): This suite should explicitly
  // generate several crash reports instead of hoping
  // that there will be enough of them already.
  describe('getLastCrashReport', () => {
    it('correctly returns the most recent report', () => {
      const reports = crashReporter.getUploadedReports()
      expect(reports).to.be.an('array')
      expect(reports).to.have.lengthOf.at.least(2,
        'There are not enough reports for this test')

      const lastReport = crashReporter.getLastCrashReport()
      expect(lastReport).to.be.an('object').that.includes.a.key('date')

      // Let's find the newest report.
      const { report: newestReport } = reports.reduce((acc, cur) => {
        const timestamp = new Date(cur.date).getTime()
        return (timestamp > acc.timestamp)
          ? { report: cur, timestamp: timestamp }
          : acc
      }, { timestamp: -Infinity })
      assert(newestReport, 'Hey!')

      expect(lastReport.date.getTime()).to.be.equal(
        newestReport.date.getTime(),
        'Last report is not the newest.')
    })
  })

  describe('getUploadToServer()', () => {
    it('throws an error when called from the renderer process', () => {
      assert.throws(() => require('electron').crashReporter.getUploadToServer())
    })
    it('returns true when uploadToServer is set to true', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes',
        uploadToServer: true
      })
      assert.strictEqual(crashReporter.getUploadToServer(), true)
    })
    it('returns false when uploadToServer is set to false', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes',
        uploadToServer: true
      })
      crashReporter.setUploadToServer(false)
      assert.strictEqual(crashReporter.getUploadToServer(), false)
    })
  })

  describe('setUploadToServer(uploadToServer)', () => {
    it('throws an error when called from the renderer process', () => {
      assert.throws(() => require('electron').crashReporter.setUploadToServer('arg'))
    })
    it('sets uploadToServer false when called with false', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes',
        uploadToServer: true
      })
      crashReporter.setUploadToServer(false)
      assert.strictEqual(crashReporter.getUploadToServer(), false)
    })
    it('sets uploadToServer true when called with true', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes',
        uploadToServer: false
      })
      crashReporter.setUploadToServer(true)
      assert.strictEqual(crashReporter.getUploadToServer(), true)
    })
  })

  describe('Parameters', () => {
    it('returns all of the current parameters', () => {
      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes'
      })

      const parameters = crashReporter.getParameters()
      assert(typeof parameters === 'object')
    })
    it('adds a parameter to current parameters', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes'
      })

      crashReporter.addExtraParameter('hello', 'world')
      assert('hello' in crashReporter.getParameters())
    })
    it('removes a parameter from current parameters', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes'
      })

      crashReporter.addExtraParameter('hello', 'world')
      assert('hello' in crashReporter.getParameters())

      crashReporter.removeExtraParameter('hello')
      assert(!('hello' in crashReporter.getParameters()))
    })
  })
})

const waitForCrashReport = () => {
  return new Promise((resolve, reject) => {
    let times = 0
    const checkForReport = () => {
      if (crashReporter.getLastCrashReport() != null) {
        resolve()
      } else if (times >= 10) {
        reject(new Error('No crash report available'))
      } else {
        times++
        setTimeout(checkForReport, 100)
      }
    }
    checkForReport()
  })
}

const startServer = ({ callback, processType, done }) => {
  let called = false
  let server = http.createServer((req, res) => {
    const form = new multiparty.Form()
    form.parse(req, (error, fields) => {
      if (error) throw error
      if (called) return
      called = true
      assert.deepStrictEqual(String(fields.prod), 'Electron')
      assert.strictEqual(String(fields.ver), process.versions.electron)
      assert.strictEqual(String(fields.process_type), processType)
      assert.strictEqual(String(fields.platform), process.platform)
      assert.strictEqual(String(fields.extra1), 'extra1')
      assert.strictEqual(String(fields.extra2), 'extra2')
      assert.strictEqual(fields.extra3, undefined)
      assert.strictEqual(String(fields._productName), 'Zombies')
      assert.strictEqual(String(fields._companyName), 'Umbrella Corporation')
      assert.strictEqual(String(fields._version), app.getVersion())

      const reportId = 'abc-123-def-456-abc-789-abc-123-abcd'
      res.end(reportId, () => {
        waitForCrashReport().then(() => {
          assert.strictEqual(crashReporter.getLastCrashReport().id, reportId)
          assert.notStrictEqual(crashReporter.getUploadedReports().length, 0)
          assert.strictEqual(crashReporter.getUploadedReports()[0].id, reportId)
          req.socket.destroy()
          done()
        }, done)
      })
    })
  })

  const activeConnections = new Set()
  server.on('connection', (connection) => {
    activeConnections.add(connection)
    connection.once('close', () => {
      activeConnections.delete(connection)
    })
  })

  let { port } = remote.process
  server.listen(port, '127.0.0.1', () => {
    port = server.address().port
    remote.process.port = port
    if (process.platform === 'darwin') {
      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1:' + port
      })
    }
    callback(port)
  })

  return function stopServer (done) {
    for (const connection of activeConnections) {
      connection.destroy()
    }
    server.close(() => {
      done()
    })
  }
}

const stopCrashService = () => {
  const { crashServicePid } = remote.process
  if (crashServicePid) {
    remote.process.crashServicePid = 0
    try {
      process.kill(crashServicePid)
    } catch (error) {
      if (error.code !== 'ESRCH') {
        throw error
      }
    }
  }
}
