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
  var fixtures = path.resolve(__dirname, 'fixtures')
  const generateSpecs = (description, browserWindowOpts) => {
    describe(description, function () {
      var w = null
      var originalTempDirectory = null
      var tempDirectory = null

      beforeEach(function () {
        w = new BrowserWindow(Object.assign({
          show: false
        }, browserWindowOpts))
        tempDirectory = temp.mkdirSync('electronCrashReporterSpec-')
        originalTempDirectory = app.getPath('temp')
        app.setPath('temp', tempDirectory)
      })

      afterEach(function () {
        app.setPath('temp', originalTempDirectory)
        return closeWindow(w).then(function () { w = null })
      })

      it('should send minidump when renderer crashes', function (done) {
        if (process.env.APPVEYOR === 'True') return done()
        if (process.env.TRAVIS === 'true') return done()

        this.timeout(120000)

        startServer({
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

        startServer({
          callback (port) {
            const crashesDir = path.join(app.getPath('temp'), `${app.getName()} Crashes`)
            const version = app.getVersion()
            const crashPath = path.join(fixtures, 'module', 'crash.js')
            childProcess.fork(crashPath, [port, version, crashesDir], {silent: true})
          },
          processType: 'browser',
          done: done
        })
      })

      it('should not send minidump if uploadToServer is false', function (done) {
        this.timeout(120000)

        if (process.platform === 'darwin') {
          crashReporter.setUploadToServer(false)
        }

        let server
        let dumpFile
        let crashesDir
        const testDone = (uploaded) => {
          if (uploaded) {
            return done(new Error('fail'))
          }
          server.close()
          if (process.platform === 'darwin') {
            crashReporter.setUploadToServer(true)
          }
          assert(fs.existsSync(dumpFile))
          fs.unlinkSync(dumpFile)
          done()
        }

        let pollInterval
        const pollDumpFile = () => {
          fs.readdir(crashesDir, (err, files) => {
            if (err) {
              return
            }
            const dumps = files.filter((file) => /\.dmp$/.test(file))
            if (!dumps.length) {
              return
            }
            assert.equal(1, dumps.length)
            dumpFile = path.join(crashesDir, dumps[0])
            clearInterval(pollInterval)
            // dump file should not be deleted when not uploading, so we wait
            // 500 ms and assert it still exists in `testDone`
            setTimeout(testDone, 500)
          })
        }

        remote.ipcMain.once('set-crash-directory', (event, dir) => {
          if (process.platform === 'linux') {
            crashesDir = dir
          } else {
            crashesDir = crashReporter.getCrashesDirectory()
            if (process.platform === 'darwin') {
              // crashpad uses an extra subdirectory
              crashesDir = path.join(crashesDir, 'completed')
            }
          }

          // Before starting, remove all dump files in the crash directory.
          // This is required because:
          // - mac crashpad not seem to allow changing the crash directory after
          //   the first "start" call.
          // - Other tests in this suite may leave dumps there.
          // - We want to verify in `testDone` that a dump file is created and
          //   not deleted.
          fs.readdir(crashesDir, (err, files) => {
            if (!err) {
              for (const file of files) {
                if (/\.dmp$/.test(file)) {
                  fs.unlinkSync(path.join(crashesDir, file))
                }
              }
            }
            event.returnValue = null  // allow the renderer to crash
            pollInterval = setInterval(pollDumpFile, 100)
          })
        })

        server = startServer({
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

        this.timeout(10000)

        startServer({
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
  generateSpecs('with sandbox ', {
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
    server.close()
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
  return server
}
