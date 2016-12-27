var fs = require('fs')
var tape = require('tape')
var path = require('path')
var childProcess = require('child_process')

tape('fails for unsupported platforms', function (t) {
  install({npm_config_platform: 'android'}, function (code, stderr) {
    t.notEqual(stderr.indexOf('Electron builds are not available on platform: android'), -1, 'has error message')
    t.equal(code, 1, 'exited with 1')
    t.end()
  })
})

tape('fails for unknown architectures', function (t) {
  install({
    npm_config_arch: 'midcentury',
    npm_config_platform: 'win32',
    HOME: process.env.HOME,
    USERPROFILE: process.env.USERPROFILE
  }, function (code, stderr) {
    t.notEqual(stderr.indexOf('Failed to find Electron'), -1, 'has error message')
    t.notEqual(stderr.indexOf('win32-midcentury'), -1, 'has error message')
    t.equal(code, 1, 'exited with 1')
    t.end()
  })
})

var install = function (env, callback) {
  removeVersionFile()

  var installPath = path.join(__dirname, '..', 'install.js')
  var installProcess = childProcess.fork(installPath, {
    silent: true,
    env: env
  })

  var stderr = ''
  installProcess.stderr.on('data', function (data) {
    stderr += data
  })

  installProcess.on('close', function (code) {
    restoreVersionFile()
    callback(code, stderr)
  })
}

var versionPath = path.join(__dirname, '..', 'dist', 'version')
var versionContents = null
function removeVersionFile () {
  if (fs.existsSync(versionPath)) {
    versionContents = fs.readFileSync(versionPath)
    fs.unlinkSync(versionPath)
  }
}

function restoreVersionFile () {
  if (versionContents != null) {
    fs.writeFileSync(versionPath, versionContents)
    versionContents = null
  }
}
