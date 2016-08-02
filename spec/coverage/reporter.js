const asar = require('asar')
const fs = require('fs')
const glob = require('glob')
const mkdirp = require('mkdirp')
const path = require('path')
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

// Generate a code coverage report in out/coverage/lcov-report
exports.generateReport = () => {
  const coverage = window.__coverage__
  if (coverage == null) return

  addUnrequiredFiles(coverage)

  const collector = new Collector()
  collector.add(coverage)

  const {ipcRenderer} = require('electron')
  collector.add(ipcRenderer.sendSync('get-coverage'))

  const reporter = new Reporter(null, outputPath)
  reporter.addAll(['text', 'lcov'])
  reporter.write(collector, true, function () {})
}
