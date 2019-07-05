const chai = require('chai')
const dirtyChai = require('dirty-chai')
const childProcess = require('child_process')
const fs = require('fs')
const http = require('http')
const multiparty = require('multiparty')
const path = require('path')
const temp = require('temp').track()
const url = require('url')
const { closeWindow } = require('./window-helpers')

const { remote } = require('electron')
const { app, BrowserWindow, crashReporter } = remote

const { expect } = chai
chai.use(dirtyChai)

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

      afterEach((done) => {
        if (stopServer != null) {
          stopServer(done)
        } else {
          done()
        }
      })

      it('should send minidump when renderer crashes', function (done) {
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
        this.timeout(180000)

        stopServer = startServer({
          callback (port) {
            const crashesDir = path.join(app.getPath('temp'), `${app.name} Crashes`)
            const version = app.getVersion()
            const crashPath = path.join(fixtures, 'module', 'crash.js')

            childProcess.fork(crashPath, [port, version, crashesDir], { silent: true })
          },
          processType: 'node',
          done: done
        })
      })

      it('should not send minidump if uploadToServer is false', function (done) {
        this.timeout(180000)

        let dumpFile
        let crashesDir = crashReporter.getCrashesDirectory()
        const existingDumpFiles = new Set()
        if (process.platform !== 'linux') {
          // crashpad puts the dump files in the "completed" subdirectory
          if (process.platform === 'darwin') {
            crashesDir = path.join(crashesDir, 'completed')
          } else {
            crashesDir = path.join(crashesDir, 'reports')
          }
          crashReporter.setUploadToServer(false)
        }
        const testDone = (uploaded) => {
          if (uploaded) return done(new Error('Uploaded crash report'))
          if (process.platform !== 'linux') crashReporter.setUploadToServer(true)
          expect(fs.existsSync(dumpFile)).to.be.true()
          done()
        }

        let pollInterval
        const pollDumpFile = () => {
          fs.readdir(crashesDir, (err, files) => {
            if (err) return
            const dumps = files.filter((file) => /\.dmp$/.test(file) && !existingDumpFiles.has(file))
            if (!dumps.length) return

            expect(dumps).to.have.lengthOf(1)
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

  generateSpecs('without sandbox', {
    webPreferences: {
      nodeIntegration: true
    }
  })
  generateSpecs('with sandbox', {
    webPreferences: {
      sandbox: true,
      preload: path.join(fixtures, 'module', 'preload-sandbox.js')
    }
  })
  generateSpecs('with remote module disabled', {
    webPreferences: {
      nodeIntegration: true,
      enableRemoteModule: false
    }
  })

  describe('getProductName', () => {
    it('returns the product name if one is specified', () => {
      const name = crashReporter.getProductName()
      const expectedName = 'Electron Test'
      expect(name).to.equal(expectedName)
    })
  })

  describe('start(options)', () => {
    it('requires that the companyName and submitURL options be specified', () => {
      expect(() => {
        crashReporter.start({ companyName: 'Missing submitURL' })
      }).to.throw('submitURL is a required option to crashReporter.start')
      expect(() => {
        crashReporter.start({ submitURL: 'Missing companyName' })
      }).to.throw('companyName is a required option to crashReporter.start')
    })
    it('can be called multiple times', () => {
      expect(() => {
        crashReporter.start({
          companyName: 'Umbrella Corporation',
          submitURL: 'http://127.0.0.1/crashes'
        })

        crashReporter.start({
          companyName: 'Umbrella Corporation 2',
          submitURL: 'http://127.0.0.1/more-crashes'
        })
      }).to.not.throw()
    })
  })

  describe('getCrashesDirectory', () => {
    it('correctly returns the directory', () => {
      const crashesDir = crashReporter.getCrashesDirectory()
      const dir = path.join(app.getPath('temp'), 'Electron Test Crashes')
      expect(crashesDir).to.equal(dir)
    })
  })

  describe('getUploadedReports', () => {
    it('returns an array of reports', () => {
      const reports = crashReporter.getUploadedReports()
      expect(reports).to.be.an('array')
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
      expect(newestReport).to.be.an('object')

      expect(lastReport.date.getTime()).to.be.equal(
        newestReport.date.getTime(),
        'Last report is not the newest.')
    })
  })

  describe('getUploadToServer()', () => {
    it('throws an error when called from the renderer process', () => {
      expect(() => require('electron').crashReporter.getUploadToServer()).to.throw()
    })
    it('returns true when uploadToServer is set to true', function () {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes',
        uploadToServer: true
      })
      expect(crashReporter.getUploadToServer()).to.be.true()
    })
    it('returns false when uploadToServer is set to false', function () {
      if (process.platform === 'linux') {
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
      expect(crashReporter.getUploadToServer()).to.be.false()
    })
  })

  describe('setUploadToServer(uploadToServer)', () => {
    it('throws an error when called from the renderer process', () => {
      expect(() => require('electron').crashReporter.setUploadToServer('arg')).to.throw()
    })
    it('sets uploadToServer false when called with false', function () {
      if (process.platform === 'linux') {
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
      expect(crashReporter.getUploadToServer()).to.be.false()
    })
    it('sets uploadToServer true when called with true', function () {
      if (process.platform === 'linux') {
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
      expect(crashReporter.getUploadToServer()).to.be.true()
    })
  })

  describe('Parameters', () => {
    it('returns all of the current parameters', () => {
      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes'
      })

      const parameters = crashReporter.getParameters()
      expect(parameters).to.be.an('object')
    })
    it('adds a parameter to current parameters', function () {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes'
      })

      crashReporter.addExtraParameter('hello', 'world')
      expect(crashReporter.getParameters()).to.have.a.property('hello')
    })
    it('removes a parameter from current parameters', function () {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      crashReporter.start({
        companyName: 'Umbrella Corporation',
        submitURL: 'http://127.0.0.1/crashes'
      })

      crashReporter.addExtraParameter('hello', 'world')
      expect(crashReporter.getParameters()).to.have.a.property('hello')

      crashReporter.removeExtraParameter('hello')
      expect(crashReporter.getParameters()).to.not.have.a.property('hello')
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
  const server = http.createServer((req, res) => {
    const form = new multiparty.Form()
    form.parse(req, (error, fields) => {
      if (error) throw error
      if (called) return
      called = true
      expect(String(fields.prod)).to.equal('Electron')
      expect(String(fields.ver)).to.equal(process.versions.electron)
      expect(String(fields.process_type)).to.equal(processType)
      expect(String(fields.platform)).to.equal(process.platform)
      expect(String(fields.extra1)).to.equal('extra1')
      expect(String(fields.extra2)).to.equal('extra2')
      expect(fields.extra3).to.be.undefined()
      expect(String(fields._productName)).to.equal('Zombies')
      expect(String(fields._companyName)).to.equal('Umbrella Corporation')
      expect(String(fields._version)).to.equal(app.getVersion())

      const reportId = 'abc-123-def-456-abc-789-abc-123-abcd'
      res.end(reportId, () => {
        waitForCrashReport().then(() => {
          expect(crashReporter.getLastCrashReport().id).to.equal(reportId)
          expect(crashReporter.getUploadedReports()).to.be.an('array').that.is.not.empty()
          expect(crashReporter.getUploadedReports()[0].id).to.equal(reportId)
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
    if (process.platform !== 'linux') {
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
