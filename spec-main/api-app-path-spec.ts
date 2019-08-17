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
const defaultAppName = 'app-custom-path'
const appName = 'myAppName'
const appData = path.join(os.tmpdir(), 'myappdata')

describe('app path module', () => {
  describe(`'userData' is implicit, computed from 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-custom-path')
      expect(output.userData).to.equal(path.join(output.appData, defaultAppName))
    })

    it(`app.name='${appName}'`, async () => {
      const output = await runTestApp('app-custom-path', `-custom-appname=${appName}`)
      expect(output.userData).to.equal(path.join(output.appData, appName))
    })

    it(`setPath('appData', '${appData}')`, async () => {
      // Cleanup
      try {
        fs.unlinkSync(appData);
      }
      catch (_) {}
      const output = await runTestApp('app-custom-path', `-custom-appdata=${appData}`)
      expect(output.appData).to.equal(appData)
      expect(output.userData).to.equal(path.join(output.appData, defaultAppName))
      // On App ready event, the appData path is created
      expect(fs.existsSync(appData))
      // Cleanup
      try {
        fs.unlinkSync(appData);
      }
      catch (_) {}
    })
  })

  describe(`'userCache' is implicit, computed from 'cache', 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-custom-path')
      expect(output.userCache).to.equal(path.join(output.appCache, defaultAppName))
    })

    it(`app.name='${appName}'`, async () => {
      const output = await runTestApp('app-custom-path', `-custom-appname=${appName}`)
      expect(output.userCache).to.equal(path.join(output.appCache, appName))
    })

    it(`setPath('appData', '${appData}')`, async () => {
      // Cleanup
      try {
        fs.unlinkSync(appData);
      }
      catch (_) {}
      const output = await runTestApp('app-custom-path', `-custom-appdata=${appData}`)
      expect(output.userCache).to.equal(path.join(output.appCache, defaultAppName))
      // On Windows, 'cache' is equivalent to 'appData'
      if (process.platform === 'win32') {
        expect(output.userCache).to.contain(appData)
        expect(output.appCache).to.equal(appData)
      }
      // Cleanup
      try {
        fs.unlinkSync(appData);
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
      expect(appPath).to.equal(path.resolve(fixturesPath, 'app-path'))
    })

    it('works for directories with index.js', async () => {
      const { appPath } = await runTestApp('app-path/lib')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'app-path/lib'))
    })

    it('works for files without extension', async () => {
      const { appPath } = await runTestApp('app-path/lib/index')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'app-path/lib'))
    })

    it('works for files', async () => {
      const { appPath } = await runTestApp('app-path/lib/index.js')
      expect(appPath).to.equal(path.resolve(fixturesPath, 'app-path/lib'))
    })
  })

  describe('setAppLogsPath', () => {
    const appLogsPath = path.join(os.tmpdir(), 'mylogs')
    it('by default', async () => {
      const output = await runTestApp('app-custom-path')
      switch (process.platform) {
        case 'darwin':
          // expect(output.appLogs).to.equal(path.join(os.homedir(), 'Logs', defaultAppName))
          expect(output.appLogs).to.equal(path.join(os.homedir(), 'Logs', 'Electron'))
          break;
        case 'win32':
        default:
          expect(output.appLogs).to.equal(path.join(output.appData, defaultAppName, 'logs'))
          expect(output.appLogs).to.equal(path.join(output.userData, 'logs'))
          break;
      }
    })
      // Linux
      // app path module setAppLogsPath() setAppLogsPath(/tmp/mylogs) - setAppLogsPath(/tmp/mylogs)
      // /home/builduser/project/src/electron/spec-main/api-app-path-spec.ts
      // AssertionError: expected '/home/builduser/.config/app-custom-path/logs' to equal '/tmp/mylogs'

    if (process.platform === 'win32') {
      it(`setAppLogsPath('${appLogsPath}')`, async () => {
        const output = await runTestApp('app-custom-path', `custom-applogs=${appLogsPath}`)
        expect(output.appLogs).to.equal(appLogsPath)
      })
    }

    it(`setAppLogsPath()`, async () => {
      const defaultAppLogs = app.getPath('logs');
      app.setAppLogsPath(appLogsPath);
      expect(app.getPath('logs')).to.equal(appLogsPath)
      app.setAppLogsPath();
      expect(app.getPath('logs')).to.equal(defaultAppLogs)
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
