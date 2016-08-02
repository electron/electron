const asar = require('asar')
const fs = require('fs')
const glob = require('glob')
const mkdirp = require('mkdirp')
const path = require('path')
const rimraf = require('rimraf')
const {Collector, Instrumenter, Reporter} = require('istanbul')

const outputPath = path.join(__dirname, '..', '..', 'out', 'coverage')
const libPath = path.join(__dirname, '..', '..', 'lib')

// Add unrequired files to the coverage report so all files are present there
const addUnrequiredFiles = (coverage) => {
  const instrumenter = new Instrumenter()
  const libPath = path.join(__dirname, '..', '..', 'lib')

  glob.sync('**/*.js', {cwd: libPath}).map(function (relativePath) {
    return path.join(libPath, relativePath)
  }).filter(function (filePath) {
    return coverage[filePath] == null
  }).forEach(function (filePath) {
    instrumenter.instrumentSync(fs.readFileSync(filePath, 'utf8'), filePath)

    // When instrumenting the code, istanbul will give each FunctionDeclaration
    // a value of 1 in coverState.s,presumably to compensate for function
    // hoisting. We need to reset this, as the function was not hoisted, as it
    // was never loaded.
    Object.keys(instrumenter.coverState.s).forEach(function (key) {
        instrumenter.coverState.s[key] = 0
    });

    coverage[filePath] = instrumenter.coverState
  })
}

// Add coverage data to collector for all opened browser windows
const addBrowserWindowsData = (collector) => {
  const dataPath = path.join(outputPath, 'data')
  glob.sync('*.json', {cwd: dataPath}).map(function (relativePath) {
    return path.join(dataPath, relativePath)
  }).forEach(function (filePath) {
    collector.add(JSON.parse(fs.readFileSync(filePath)));
  })
}

// Generate a code coverage report in out/coverage/lcov-report
exports.generateReport = () => {
  const coverage = window.__coverage__
  if (coverage == null) return

  addUnrequiredFiles(coverage)

  const collector = new Collector()
  collector.add(coverage)

  const {ipcRenderer} = require('electron')
  collector.add(ipcRenderer.sendSync('get-coverage'))

  addBrowserWindowsData(collector)

  const reporter = new Reporter(null, outputPath)
  reporter.addAll(['text', 'lcov'])
  reporter.write(collector, true, function () {})
}

// Save coverage data from the browser window with the given pid
const saveCoverageData = (webContents, coverage, pid) => {
  if (coverage && pid) {
    const dataPath = path.join(outputPath, 'data', `${pid || webContents.getId()}-${webContents.getType()}-${Date.now()}.json`)
    mkdirp.sync(path.dirname(dataPath))
    fs.writeFileSync(dataPath, JSON.stringify(coverage))
  }
}

const getCoverageFromWebContents = (webContents, callback) => {
  webContents.executeJavaScript('[window.__coverage__, window.process && window.process.pid]', (results) => {
    const coverage = results[0]
    const pid = results[1]
    callback(coverage, pid)
  })
}

// Save coverage data when a BrowserWindow is closed manually
const patchBrowserWindow = () => {
  const {BrowserWindow} = require('electron')

  const {destroy} = BrowserWindow.prototype
  BrowserWindow.prototype.destroy = function () {
    if (this.isDestroyed() || !this.getURL()) {
      return destroy.call(this)
    }

    getCoverageFromWebContents(this.webContents, (coverage, pid) => {
      saveCoverageData(this.webContents, coverage, pid)
      destroy.call(this)
    })
  }
}

// Save coverage data when beforeunload fires on the webContent's window object
const saveCoverageOnBeforeUnload = () => {
  const {app, ipcMain} = require('electron')

  ipcMain.on('save-coverage', function (event, coverage, pid) {
    saveCoverageData(event.sender, coverage, pid)
  })

  app.on('web-contents-created', function (event, webContents) {
    webContents.executeJavaScript(`
      window.addEventListener('beforeunload', function () {
        require('electron').ipcRenderer.send('save-coverage', window.__coverage__, window.process && window.process.pid)
      })
    `)
  })
}

exports.setupCoverage = () => {
  const coverage = global.__coverage__
  if (coverage == null) return

  rimraf.sync(path.join(outputPath, 'data'))
  patchBrowserWindow()
  saveCoverageOnBeforeUnload()
}
