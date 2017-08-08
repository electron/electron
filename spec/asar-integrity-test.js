const path = require('path')
const plist = require('plist')
const fs = require('fs')
const { computeData } = require('asar-integrity')
const { execFileSync } = require('child_process')

const testAppAsar = Buffer.from('BAAAAGQAAABgAAAAWgAAAHsiZmlsZXMiOnsiaW5kZXguanMiOnsic2l6ZSI6MjQxLCJvZmZzZXQiOiIwIn0sInBhY2thZ2UuanNvbiI6eyJzaXplIjo4NCwib2Zmc2V0IjoiMjQxIn19fQAAJ3VzZSBzdHJpY3QnCgpjb25zdCB7IGFwcCB9ID0gcmVxdWlyZSgnZWxlY3Ryb24nKQpjb25zdCB7IGpvaW4gfSA9IHJlcXVpcmUoJ3BhdGgnKQoKYXBwLm9uKCdyZWFkeScsICgpID0+IHsKICBpZiAocHJvY2Vzcy5lbnYuVEVTVF9SRVFVSVJFX0VYVEVSTkFMKSB7CiAgICByZXF1aXJlKGpvaW4ocHJvY2Vzcy5yZXNvdXJjZXNQYXRoLCAnLi4nLCAnZXh0ZXJuYWwuYXNhcicpKQogIH0KCiAgcHJvY2Vzcy5leGl0KDApCn0pCnsKICAibmFtZSI6ICJUZXN0QXBwIiwKICAidmVyc2lvbiI6ICIxLjAuMCIsCiAgImxpY2Vuc2UiOiAiTUlUIiwKICAiYXV0aG9yIjogIkZvbyIKfQ==', 'base64')

describe('Asar Integrity', function () {
  if (process.platform !== 'darwin') {
    return
  }

  const originalAppDir = path.join(__dirname, '../out/D/Electron.app')
  const appDir = path.join(__dirname, '../out/test/Electron.app')
  execFileSync('rm', ['-rf', appDir])
  execFileSync('mkdir', ['-p', appDir])
  execFileSync('cp', ['-R', originalAppDir + '/', appDir])

  const resourcesDir = path.join(appDir, 'Contents/Resources')
  fs.writeFileSync(path.join(resourcesDir, 'app.asar'), testAppAsar)

  const testEnv = Object.assign({}, process.env, {ELECTRON_DEBUG_ASAR_CHECK: 'true', ELECTRON_TEST_ASAR_CHECK: 'true'})

  // test that works correctly without AsarIntegrity
  const runApp = () => {
    execFileSync(path.join(appDir, 'Contents/MacOS/Electron'), {
      timeout: 30 * 1000,
      env: testEnv,
      stdio: ['ignore', 'pipe', 'pipe']
    })
  }

  const infoPlistFile = path.join(appDir, 'Contents/Info.plist')
  const manifest = plist.parse(fs.readFileSync(infoPlistFile, 'utf8'))

  let asarIntegrity

  // https://github.com/mochajs/mocha/issues/1431
  before(() => computeData(resourcesDir)
    .then(it => {
      asarIntegrity = it
    }))

  function saveManifest (data) {
    manifest.AsarIntegrity = JSON.stringify(data)
    fs.writeFileSync(infoPlistFile, plist.build(manifest))
  }

  const runExternalAsarTest = (task) => {
    const externalFile = path.join(resourcesDir, '..', 'external.asar')
    // file must exists
    fs.writeFileSync(externalFile, testAppAsar)
    try {
      testEnv.TEST_REQUIRE_EXTERNAL = 'true'
      task()
    } finally {
      delete testEnv.TEST_REQUIRE_EXTERNAL
      fs.unlinkSync(externalFile)
    }
  }

  it('without AsarIntegrity', () => runApp())

  it('with valid AsarIntegrity', () => {
    saveManifest(asarIntegrity)
    runApp()
  })

  it('invalid checksum', () => {
    saveManifest(Object.assign({}, asarIntegrity, {checksums: Object.assign({}, asarIntegrity.checksums, {'app.asar': 'invalid checksum'})}))
    expectError(runApp, 'is corrupted, checksum mismatch')
  })

  it('load external if disallowed', () => {
    saveManifest(asarIntegrity)
    runExternalAsarTest(() => expectError(runApp, 'but loading of external files is disallowed'))
  })

  it('load external if allowed', () => {
    saveManifest(Object.assign({}, asarIntegrity, {externalAllowed: true}))
    runExternalAsarTest(runApp)
  })
})

function expectError (task, s) {
  let errorOccurred = false
  try {
    task()
  } catch (e) {
    errorOccurred = true
    if (!e.message.includes(s)) {
      throw e
    }
  }

  if (!errorOccurred) {
    throw new Error(`Error expected: ${s}`)
  }
}
