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
const defaultAppName = 'app-user-paths'

describe('app paths module', () => {
  describe(`'userData' is computed from 'appData' and app name`, () => {
    it('by default', async () => {
      const output = await runTestApp('app-user-paths')
      expect(output.userData).to.equal(path.join(output.appData, defaultAppName))
    })

    it('customizes app name', async () => {
      const appName = 'myAppName'
      const output = await runTestApp('app-user-paths', `-custom-appname=${appName}`)
      expect(output.userData).to.equal(path.join(output.appData, appName))
    })

    it('customizes appData', async () => {
      const appdata = path.join(os.tmpdir(), 'myappdata')
      // Cleanup
      try {
        fs.unlinkSync(appdata);
      }
      catch (_) {}
      const output = await runTestApp('app-user-paths', `-custom-appdata=${appdata}`)
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
      const output = await runTestApp('app-user-paths')
      expect(output.userCache).to.equal(path.join(output.appCache, defaultAppName))
    })

    it('customizes app name', async () => {
      const appName = 'myAppName'
      const output = await runTestApp('app-user-paths', `-custom-appname=${appName}`)
      expect(output.userCache).to.equal(path.join(output.appCache, appName))
    })

    it('customizes appData', async () => {
      const appdata = path.join(os.tmpdir(), 'myappdata')
      // Cleanup
      try {
        fs.unlinkSync(appdata);
      }
      catch (_) {}
      const output = await runTestApp('app-user-paths', `-custom-appdata=${appdata}`)
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
