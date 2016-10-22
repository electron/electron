const assert = require('assert')
const http = require('http')
const multiparty = require('multiparty')
const path = require('path')
const temp = require('temp').track()
const url = require('url')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {app, BrowserWindow, crashReporter} = remote.require('electron')

describe('crashReporter module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  var w = null
  var originalTempDirectory = null
  var tempDirectory = null

  beforeEach(function () {
    w = new BrowserWindow({
      show: false
    })
    tempDirectory = temp.mkdirSync('electronCrashReporterSpec-')
    originalTempDirectory = app.getPath('temp')
    app.setPath('temp', tempDirectory)
  })

  afterEach(function () {
    app.setPath('temp', originalTempDirectory)
    return closeWindow(w).then(function () { w = null })
  })

  if (process.mas) {
    return
  }

  var isCI = remote.getGlobal('isCi')
  if (isCI) {
    return
  }

  it('should send minidump when renderer crashes', function (done) {
    this.timeout(120000)

    var called = false
    var server = http.createServer(function (req, res) {
      server.close()
      var form = new multiparty.Form()
      form.parse(req, function (error, fields) {
        if (error) throw error
        if (called) return
        called = true
        assert.equal(fields['prod'], 'Electron')
        assert.equal(fields['ver'], process.versions.electron)
        assert.equal(fields['process_type'], 'renderer')
        assert.equal(fields['platform'], process.platform)
        assert.equal(fields['extra1'], 'extra1')
        assert.equal(fields['extra2'], 'extra2')
        assert.equal(fields['_productName'], 'Zombies')
        assert.equal(fields['_companyName'], 'Umbrella Corporation')
        assert.equal(fields['_version'], app.getVersion())

        const reportId = 'abc-123-def-456-abc-789-abc-123-abcd'
        res.end(reportId, () => {
          waitForCrashReport().then(() => {
            assert.equal(crashReporter.getLastCrashReport().id, reportId)
            assert.notEqual(crashReporter.getUploadedReports().length, 0)
            assert.equal(crashReporter.getUploadedReports()[0].id, reportId)
            done()
          }, done)
        })
      })
    })
    var port = remote.process.port
    server.listen(port, '127.0.0.1', function () {
      port = server.address().port
      remote.process.port = port
      const crashUrl = url.format({
        protocol: 'file',
        pathname: path.join(fixtures, 'api', 'crash.html'),
        search: '?port=' + port
      })
      if (process.platform === 'darwin') {
        crashReporter.start({
          companyName: 'Umbrella Corporation',
          submitURL: 'http://127.0.0.1:' + port
        })
      }
      w.loadURL(crashUrl)
    })
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
