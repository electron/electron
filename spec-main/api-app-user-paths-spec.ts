import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as cp from 'child_process'
import * as path from 'path'
import * as os from 'os'
import { emittedOnce } from './events-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.join(__dirname, 'fixtures', 'api')
const defaultAppName = 'app-user-paths'

describe('app user paths module', () => {
  describe('app paths customization', () => {
    it('by default', async () => {
      const output = await runTestApp('app-user-paths')
      const expected = path.join(output.appData, defaultAppName)
      expect(output.userCache).to.equal(expected)
      expect(output.userData).to.equal(expected)
    })

    it('customize app name', async () => {
      const appName = 'myAppName'
      const output = await runTestApp('app-user-paths', `-custom-appname=${appName}`)
      const expected = path.join(output.appData, appName)
      expect(output.userCache).to.equal(expected)
      expect(output.userData).to.equal(expected)
    })

    it('customize appData', async () => {
      const appdata = path.join(os.tmpdir(), 'myappdata')
      const output = await runTestApp('app-user-paths', `-custom-appdata=${appdata}`)
      const expected = path.join(output.appData, defaultAppName)
      expect(output.appData).to.equal(appdata)
      expect(output.userCache).to.equal(expected)
      expect(output.userData).to.equal(expected)
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

  return JSON.parse(output)
}
