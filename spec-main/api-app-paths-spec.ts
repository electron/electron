import { app } from 'electron'
import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as cp from 'child_process'
import * as fs from 'fs'
import * as path from 'path'
import * as os from 'os'
import { emittedOnce } from './events-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.join(__dirname, 'fixtures', 'api')
const defaultAppName = 'app-paths'

describe('app paths module', () => {
  describe(`'userData' is computed from 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-paths')
      expect(output.userData).to.equal(path.join(output.appData, defaultAppName))
    })

    it('customizes app name', async () => {
      const appName = 'myAppName'
      const output = await runTestApp('app-paths', `-custom-appname=${appName}`)
      expect(output.userData).to.equal(path.join(output.appData, appName))
    })

    it('customizes appData', async () => {
      const appdata = path.join(os.tmpdir(), 'myappdata')
      // Cleanup
      try {
        fs.unlinkSync(appdata);
      }
      catch (_) {}
      const output = await runTestApp('app-paths', `-custom-appdata=${appdata}`)
      expect(output.appData).to.equal(appdata)
      expect(output.userData).to.equal(path.join(output.appData, defaultAppName))
      // On App ready event, the appdata path is created
      expect(fs.existsSync(appdata))
      // Cleanup
      try {
        fs.unlinkSync(appdata);
      }
      catch (_) {}
    })
  })

  describe(`'userCache' is computed from 'cache', 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-paths')
      expect(output.userCache).to.equal(path.join(output.appCache, defaultAppName))
    })

    it('customizes app name', async () => {
      const appName = 'myAppName'
      const output = await runTestApp('app-paths', `-custom-appname=${appName}`)
      expect(output.userCache).to.equal(path.join(output.appCache, appName))
    })

    it('customizes appData', async () => {
      const appdata = path.join(os.tmpdir(), 'myappdata')
      // Cleanup
      try {
        fs.unlinkSync(appdata);
      }
      catch (_) {}
      const output = await runTestApp('app-paths', `-custom-appdata=${appdata}`)
      expect(output.userCache).to.equal(path.join(output.appCache, defaultAppName))
      // On Windows, 'cache' is equivalent to 'appData'
      if (process.platform === 'win32') {
        expect(output.userCache).to.contain(appdata)
        expect(output.appCache).to.equal(appdata)
      }
      // Cleanup
      try {
        fs.unlinkSync(appdata);
      }
      catch (_) {}
    })
  })

  describe('getPath(name)', () => {
    it('returns paths that exist', () => {
      const paths = [
        fs.existsSync(app.getPath('exe')),
        fs.existsSync(app.getPath('home')),
        fs.existsSync(app.getPath('temp'))
      ]
      expect(paths).to.deep.equal([true, true, true])
    })

    it('throws an error when the name is invalid', () => {
      expect(() => {
        app.getPath('does-not-exist' as any)
      }).to.throw(/Failed to get 'does-not-exist' path/)
    })

    it('returns the overridden path', () => {
      app.setPath('music', __dirname)
      expect(app.getPath('music')).to.equal(__dirname)
    })
  })

  describe('setPath(name, path)', () => {
    it('does not create a new directory by default', () => {
      const badPath = path.join(__dirname, 'music')

      expect(fs.existsSync(badPath)).to.be.false
      app.setPath('music', badPath)
      expect(fs.existsSync(badPath)).to.be.false

      expect(() => { app.getPath(badPath as any) }).to.throw()
    })
  })

  describe('getAppPath', () => {
    it('works for directories with package.json', async () => {
      const { appPath } = await runTestApp('app-path')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'api/app-path'))
    })

    it('works for directories with index.js', async () => {
      const { appPath } = await runTestApp('app-path/lib')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'api/app-path/lib'))
    })

    it('works for files without extension', async () => {
      const { appPath } = await runTestApp('app-path/lib/index')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'api/app-path/lib'))
    })

    it('works for files', async () => {
      const { appPath } = await runTestApp('app-path/lib/index.js')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'api/app-path/lib'))
    })
  })

})


async function runTestApp (name: string, ...args: any[]) {
  const appPath = path.join(fixturesPath, name)
  const electronPath = process.execPath
  const appProcess = cp.spawn(electronPath, [appPath, ...args])

  let output = ''
  appProcess.stdout.on('data', (data) => { output += data })

  await emittedOnce(appProcess.stdout, 'end')

  // console.log(`ouput=${output}`)

  return JSON.parse(output)
}
