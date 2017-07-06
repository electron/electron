var fs = require('fs')
var tape = require('tape')
var path = require('path')
var childProcess = require('child_process')

tape('install fails for unsupported platforms', function (t) {
  install({npm_config_platform: 'android'}, function (code, stderr) {
    t.notEqual(stderr.indexOf('Electron builds are not available on platform: android'), -1, 'has error message')
    t.notEqual(code, 0, 'exited with error')
    t.end()
  })
})

tape('install fails for unsupported architectures', function (t) {
  install({
    npm_config_arch: 'midcentury',
    npm_config_platform: 'win32',
    HOME: process.env.HOME,
    USERPROFILE: process.env.USERPROFILE
  }, function (code, stderr) {
    t.notEqual(stderr.indexOf('Failed to find Electron'), -1, 'has error message')
    t.notEqual(stderr.indexOf('win32-midcentury'), -1, 'has error message')
    t.notEqual(code, 0, 'exited with error')
    t.end()
  })
})

tape('require fails for corrupted installs', function (t) {
  cli(function (code, stderr) {
    t.notEqual(stderr.indexOf('Electron failed to install correctly'), -1, 'has error message')
    t.notEqual(code, 0, 'exited with error')
    t.end()
  })
})

function install (env, callback) {
  var restoreFile = removeFile(path.join(__dirname, '..', 'dist', 'version'))

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
    restoreFile()
    callback(code, stderr)
  })
}

function cli (callback) {
  var restoreFile = removeFile(path.join(__dirname, '..', 'path.txt'))

  var cliPath = path.join(__dirname, '..', 'cli.js')
  var cliProcess = childProcess.fork(cliPath, {
    silent: true
  })

  var stderr = ''
  cliProcess.stderr.on('data', function (data) {
    stderr += data
  })

  cliProcess.on('close', function (code) {
    restoreFile()
    callback(code, stderr)
  })
}

function removeFile (filePath) {
  var contents = null
  if (fs.existsSync(filePath)) {
    contents = fs.readFileSync(filePath)
    fs.unlinkSync(filePath)
  }
  return function restoreFile () {
    if (contents != null) {
      fs.writeFileSync(filePath, contents)
    }
  }
}
