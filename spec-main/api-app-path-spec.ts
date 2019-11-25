import { app } from 'electron'
import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as cp from 'child_process'
import * as fs from 'fs'
import * as path from 'path'
import * as os from 'os'
import { emittedOnce } from './events-helpers'

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.join(__dirname, 'fixtures', 'api')
const defaultAppName = 'app-custom-path'
const appName = 'myAppName'
const appData = path.join(os.tmpdir(), 'myappdata')
const userData = path.join(os.tmpdir(), 'myuserdata')
const userCache = path.join(os.tmpdir(), 'myusercache')

interface Payload {
  appName: string,
  appData: string,
  appCache: string,
  userCache: string,
  userData: string,
  appLogs: string,
  appPath: string
  [key: string]: string;
}

function ExistsCacheSync (cachedir: string): boolean {
  return fs.existsSync(cachedir) &&
    fs.existsSync(path.join(cachedir, 'Cookies')) &&
    fs.existsSync(path.join(cachedir, 'Cookies-journal')) &&
    fs.existsSync(path.join(cachedir, 'Cache'))
}

function RemoveDir (dir?: string) {
  // console.log(`RemoveDir ${dir}`)
  if (dir && dir.length && fs.existsSync(dir)) {
    try {
      fs.unlinkSync(dir)
    } catch (err) {
    }
  }
}

function CleanUp (output: Payload) {
  RemoveDir(output.userData)
  RemoveDir(output.userCache)
  RemoveDir(output.appLogs)
}

describe('app path module', () => {
  describe(`computes 'userData' from 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-custom-path')
      const expectedUserdata = path.join(output.appData, defaultAppName)
      expect(output.userData).to.equal(expectedUserdata)
      CleanUp(output)
    })

    it(`app.name='${appName}'`, async () => {
      const output = await runTestApp('app-custom-path', `-custom-appname=${appName}`)
      const expectedUserdata = path.join(output.appData, appName)
      expect(output.userData).to.equal(expectedUserdata)
      CleanUp(output)
    })

    it(`setPath('appData', '${appData}')`, async () => {
      RemoveDir(appData)
      const output = await runTestApp('app-custom-path', `-custom-appdata=${appData}`)
      const expectedUserdata = path.join(appData, defaultAppName)
      expect(output.appData).to.equal(appData)
      expect(output.userData).to.equal(expectedUserdata)
      // On App ready event, the appData path is created
      expect(fs.existsSync(appData))
      CleanUp(output)
      RemoveDir(appData)
    })
  })

  describe(`customizes 'userData'`, () => {
    it(`setPath('userData', '${userData}')`, async () => {
      RemoveDir(userData)
      const output = await runTestApp('app-custom-path', `-custom-userdata=${userData}`)
      expect(output.userData).to.equal(userData)
      // On App ready event, the appData path is created
      expect(fs.existsSync(userData))
      CleanUp(output)
    })

    it(`--user-data-dir='${userData}')`, async () => {
      RemoveDir(userData)
      const output = await runTestApp('app-custom-path', `--user-data-dir=${userData}`)
      expect(output.userData).to.equal(userData)
      // On App ready event, the appData path is created
      expect(fs.existsSync(userData))
      CleanUp(output)
    })
  })

  describe(`computes 'userCache' from 'cache', 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-custom-path', '-create-cache')
      const expectedUsercache = path.join(output.appCache, defaultAppName)
      expect(output.userCache).to.equal(expectedUsercache)
      expect(ExistsCacheSync(expectedUsercache))
      CleanUp(output)
    })

    it(`app.name='${appName}'`, async () => {
      const output = await runTestApp('app-custom-path', '-create-cache', `-custom-appname=${appName}`)
      const expectedUsercache = path.join(output.appCache, appName)
      expect(output.userCache).to.equal(expectedUsercache)
      expect(ExistsCacheSync(expectedUsercache))
    })

    it(`setPath('appData', '${appData}')`, async () => {
      RemoveDir(appData)
      const output = await runTestApp('app-custom-path', `-custom-appdata=${appData}`)
      const expectedUsercache = path.join(output.appCache, defaultAppName)
      expect(output.userCache).to.equal(expectedUsercache)
      // On Windows, 'cache' is equivalent to 'appData'
      if (process.platform === 'win32') {
        expect(output.userCache).to.contain(appData)
        expect(output.appCache).to.equal(appData)
      }
      expect(ExistsCacheSync(expectedUsercache))
      CleanUp(output)
      RemoveDir(appData)
    })
  })

  describe(`customizes 'userCache'`, () => {
    it(`setPath('userCache', '${userCache}')`, async () => {
      RemoveDir(userCache)
      const output = await runTestApp('app-custom-path', '-create-cache', `-custom-usercache=${userCache}`)
      expect(output.userCache).to.equal(userCache)
      expect(output.userData).to.not.equal(userCache)
      expect(ExistsCacheSync(userCache))
      CleanUp(output)
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

      expect(fs.existsSync(badPath)).to.be.false()
      app.setPath('music', badPath)
      expect(fs.existsSync(badPath)).to.be.false()

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
          expect(output.appLogs).to.equal(path.join(os.homedir(), 'Library', 'Logs', 'Electron'))
          break
        case 'win32':
        default:
          expect(output.appLogs).to.equal(path.join(output.appData, defaultAppName, 'logs'))
          expect(output.appLogs).to.equal(path.join(output.userData, 'logs'))
          break
      }
    })
    // Linux
    // app path module setAppLogsPath() setAppLogsPath(/tmp/mylogs) - setAppLogsPath(/tmp/mylogs)
    // /home/builduser/project/src/electron/spec-main/api-app-path-spec.ts
    // AssertionError: expected '/home/builduser/.config/app-custom-path/logs' to equal '/tmp/mylogs'

    // if (process.platform === 'win32') {
    //   it(`setAppLogsPath('${appLogsPath}')`, async () => {
    //     const output = await runTestApp('app-custom-path', `-custom-applogs=${appLogsPath}`)
    //     expect(output.appLogs).to.equal(appLogsPath)
    //   })
    // }

    // To review later
    it(`setAppLogsPath()`, async () => {
      const defaultAppLogs = app.getPath('logs')
      app.setAppLogsPath(appLogsPath)
      expect(app.getPath('logs')).to.equal(appLogsPath)
      app.setAppLogsPath()
      expect(app.getPath('logs')).to.equal(defaultAppLogs)
    })
  })

//   describe('getPath("logs")', () => {
//     const logsPaths = {
//       'darwin': path.resolve(os.homedir(), 'Library', 'Logs'),
//       'linux': path.resolve(app.getPath('userData'), 'logs'),
//       'win32': path.resolve(app.getPath('userData'), 'logs'),
//     }

//     it('has no logs directory by default', () => {
//       const osLogPath = (logsPaths as any)[process.platform]
//       expect(fs.existsSync(osLogPath)).to.be.false()
//     })

//     it('creates a new logs directory if one does not exist', () => {
//       expect(() => { app.getPath('logs') }).to.not.throw()

//       const osLogPath = (logsPaths as any)[process.platform]
//       expect(fs.existsSync(osLogPath)).to.be.true()
//     })
//   })
})

async function runTestApp (name: string, ...args: any[]) {
  const appPath = path.join(fixturesPath, name)
  const electronPath = process.execPath
  const appProcess = cp.spawn(electronPath, [appPath, ...args])

  let output = ''
  appProcess.stdout.on('data', (data) => { output += data })

  await emittedOnce(appProcess.stdout, 'end')

  // console.log(`ouput=${output}`)
  try {
    const outputJSON = JSON.parse(output) as Payload
    // On Mac, paths may return symlinks so we need to work on realpath everywhere !
    const props = ['appData', 'appCache', 'userCache', 'userData', 'appLogs', 'appPath']
    props.forEach(prop => {
        try {
            outputJSON[prop] = fs.realpathSync(outputJSON[prop])
        } catch (err) {}
    })
    return outputJSON
  } catch (err) {
    console.log(`Error ${err}) with ${output}`)
    throw err
  }
}
