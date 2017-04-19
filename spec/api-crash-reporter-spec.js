const assert = require('assert')
const childProcess = require('child_process')
const fs = require('fs')
const http = require('http')
const multiparty = require('multiparty')
const path = require('path')
const temp = require('temp').track()
const url = require('url')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {app, BrowserWindow, crashReporter} = remote.require('electron')

describe('crashReporter module', function () {
  if (process.mas) {
    return
  }

  var originalTempDirectory = null
  var tempDirectory = null

  before(function () {
    tempDirectory = temp.mkdirSync('electronCrashReporterSpec-')
    originalTempDirectory = app.getPath('temp')
    app.setPath('temp', tempDirectory)
  })

  after(function () {
    app.setPath('temp', originalTempDirectory)
  })

  var fixtures = path.resolve(__dirname, 'fixtures')
  const generateSpecs = (description, browserWindowOpts) => {
    describe(description, function () {
      var w = null
      var stopServer = null

      beforeEach(function () {
        stopServer = null
        w = new BrowserWindow(Object.assign({
          show: false
        }, browserWindowOpts))
      })

      afterEach(function () {
        return closeWindow(w).then(function () { w = null })
      })

      afterEach(function () {
        stopCrashService()
      })

      afterEach(function (done) {
        if (stopServer != null) {
          stopServer(done)
        } else {
          done()
        }
      })

      it('should send minidump when renderer crashes', function (done) {
        if (process.env.APPVEYOR === 'True') return done()
        if (process.env.TRAVIS === 'true') return done()

        this.timeout(120000)

        stopServer = startServer({
          callback (port) {
            const crashUrl = url.format({
              protocol: 'file',
              pathname: path.join(fixtures, 'api', 'crash.html'),
              search: '?port=' + port
            })
            w.loadURL(crashUrl)
          },
          processType: 'renderer',
          done: done
        })
      })

      it('should send minidump when node processes crash', function (done) {
        if (process.env.APPVEYOR === 'True') return done()
        if (process.env.TRAVIS === 'true') return done()

        this.timeout(120000)

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

            childProcess.fork(crashPath, [port, version, crashesDir], {silent: true})
          },
          processType: 'browser',
          done: done
        })
      })

      it('should not send minidump if uploadToServer is false', function (done) {
        this.timeout(120000)

        let dumpFile
        let crashesDir = crashReporter.getCrashesDirectory()
        const existingDumpFiles = new Set()
        if (process.platform === 'darwin') {
          // crashpad puts the dump files in the "completed" subdirectory
          crashesDir = path.join(crashesDir, 'completed')
          crashReporter.setUploadToServer(false)
        }
        const testDone = (uploaded) => {
          if (uploaded) {
            return done(new Error('Uploaded crash report'))
          }
          if (process.platform === 'darwin') {
            crashReporter.setUploadToServer(true)
          }
          assert(fs.existsSync(dumpFile))
          done()
        }

        let pollInterval
        const pollDumpFile = () => {
          fs.readdir(crashesDir, (err, files) => {
            if (err) {
              return
            }
            const dumps = files.filter((file) => /\.dmp$/.test(file) && !existingDumpFiles.has(file))
            if (!dumps.length) {
              return
            }
            assert.equal(1, dumps.length)
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
            event.returnValue = null  // allow the renderer to crash
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
        if (process.env.APPVEYOR === 'True') return done()
        if (process.env.TRAVIS === 'true') return done()

        this.timeout(120000)

        stopServer = startServer({
          callback (port) {
            const crashUrl = url.format({
              protocol: 'file',
              pathname: path.join(fixtures, 'api', 'crash-restart.html'),
              search: '?port=' + port
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

  describe('.start(options)', function () {
    it('requires that the companyName and submitURL options be specified', function () {
      assert.throws(function () {
        crashReporter.start({
          companyName: 'Missing submitURL'
        })
      }, /submitURL is a required option to crashReporter\.start/)
      assert.throws(function () {
        crashReporter.start({
          submitURL: 'Missing companyName'
        })
      }, /companyName is a required option to crashReporter\.start/)
    })

    it('can be called multiple times', function () {
      assert.doesNotThrow(function () {
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

  describe('.get/setUploadToServer', function () {
    it('throws an error when called from the renderer process', function () {
      assert.throws(() => require('electron').crashReporter.getUploadToServer())
    })

    it('can be read/set from the main process', function () {
      if (process.platform === 'darwin') {
        crashReporter.start({
          companyName: 'Umbrella Corporation',
          submitURL: 'http://127.0.0.1/crashes',
          uploadToServer: true
        })
        assert.equal(crashReporter.getUploadToServer(), true)
        crashReporter.setUploadToServer(false)
        assert.equal(crashReporter.getUploadToServer(), false)
      } else {
        assert.equal(crashReporter.getUploadToServer(), true)
      }
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

const startServer = ({callback, processType, done}) => {
  var called = false
  var server = http.createServer((req, res) => {
    var form = new multiparty.Form()
    form.parse(req, (error, fields) => {
      if (error) throw error
      if (called) return
      called = true
      assert.equal(fields.prod, 'Electron')
      assert.equal(fields.ver, process.versions.electron)
      assert.equal(fields.process_type, processType)
      assert.equal(fields.platform, process.platform)
      assert.equal(fields.extra1, 'extra1')
      assert.equal(fields.extra2, 'extra2')
      assert.equal(fields.extra3, undefined)
      assert.equal(fields._productName, 'Zombies')
      assert.equal(fields._companyName, 'Umbrella Corporation')
      assert.equal(fields._version, app.getVersion())

      const reportId = 'abc-123-def-456-abc-789-abc-123-abcd'
      res.end(reportId, () => {
        waitForCrashReport().then(() => {
          assert.equal(crashReporter.getLastCrashReport().id, reportId)
          assert.notEqual(crashReporter.getUploadedReports().length, 0)
          assert.equal(crashReporter.getUploadedReports()[0].id, reportId)
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

  let {port} = remote.process
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
    server.close(function () {
      done()
    })
  }
}

const stopCrashService = () => {
  const {crashServicePid} = remote.process
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
