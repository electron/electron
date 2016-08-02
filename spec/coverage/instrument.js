// Generate an instrumented .asar file for all the files in lib/ and save it
// to out/coverage/electron-instrumented.asar

var asar = require('asar')
var fs = require('fs')
var glob = require('glob')
var Instrumenter = require('istanbul').Instrumenter
var mkdirp = require('mkdirp')
var path = require('path')
var rimraf = require('rimraf')

var instrumenter = new Instrumenter()
var outputPath = path.join(__dirname, '..', '..', 'out', 'coverage')
var libPath = path.join(__dirname, '..', '..', 'lib')

rimraf.sync(path.join(outputPath, 'lib'))

glob.sync('**/*.js', {cwd: libPath}).forEach(function (relativePath) {
  var rawPath = path.join(libPath, relativePath)
  var raw = fs.readFileSync(rawPath, 'utf8')

  var generatedPath = path.join(outputPath, 'lib', relativePath)
  var generated = instrumenter.instrumentSync(raw, rawPath)
  mkdirp.sync(path.dirname(generatedPath))
  fs.writeFileSync(generatedPath, generated)
})

var asarPath = path.join(outputPath, 'electron.asar')
asar.createPackageWithOptions(path.join(outputPath, 'lib'), asarPath, {}, function (error) {
  if (error) {
    console.error(error.stack || error)
    process.exit(1)
  }
})
