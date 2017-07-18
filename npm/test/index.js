const tape = require('tape')
const proxyquire = require('proxyquire')
const path = require('path')
const sinon = require('sinon')
const admZip = require('adm-zip')
const temp = require('temp')
// var pathExists = require('path-exists')
// var getHomePath = require('home-path')()

let sandbox
const mockEnv = {
  electron_config_cache: 'cache',
  npm_config_platform: 'linux',
  npm_config_arch: 'win32',
  npm_config_strict_ssl: 'true',
  force_no_cache: 'false',
  npm_config_loglevel: 'silly'
}
let tempDir
temp.track()

tape('set up', (t) => {
  sandbox = sinon.sandbox.create()
  tempDir = temp.mkdirSync('electron-install')
  console.log(tempDir)
  t.end()
})

tape('download electron', (t) => {
  const downloadSpy = sinon.spy()

  sandbox.stub(process, 'env').value(mockEnv)
  proxyquire(path.join(__dirname, '..', 'install.js'), {
    'electron-download': downloadSpy
  })
  t.ok(downloadSpy.calledWith({
    cache: mockEnv.electron_config_cache,
    version: require('../../package').version.replace(/-.*/, ''),
    platform: mockEnv.npm_config_platform,
    arch: mockEnv.npm_config_arch,
    strictSSL: mockEnv.npm_config_strict_ssl === 'true',
    force: mockEnv.force_no_cache === 'true',
    quiet: false
  }), 'electron-download is called with correct options')

  t.end()
})

tape('fails for unsupported platforms', (t) => {
  sandbox.restore()
  sandbox.stub(process, 'env').value(
    Object.assign(mockEnv, { npm_config_platform: 'android' })
  )
  t.throws(() => {
    proxyquire(path.join(__dirname, '..', 'install.js'), {
      'electron-download': sinon.spy()
    })
  },
  /Electron builds are not available on platform: android/i,
  'install fails for unsupported platforms')
  t.end()
})

tape('extract file', (t) => {

  sandbox.restore()

  sandbox.stub(process, 'env').value(
    Object.assign(mockEnv, { npm_config_platform: 'darwin' })
  )

  // add file directly
  const zip = new admZip()
  zip.addFile('test.txt', Buffer.from('electron install test'))
  zip.writeZip(path.join(tempDir, 'test.zip'))

  // create fake zip
  // mock download() returning path to fake zip

  proxyquire(path.join(__dirname, '..', 'install.js'), {
    'electron-download': (opts, cb) => cb(null, path.join(tempDir, 'test.zip'))
  })

  // call through to extractFile()
  // check `/path.txt` to contain platformPath
  t.end()
})

tape('teardown', (t) => {
  // remove files
  t.end()
})
