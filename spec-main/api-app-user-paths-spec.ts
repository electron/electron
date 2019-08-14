import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as cp from 'child_process'
import * as path from 'path'
import { app } from 'electron'
import { emittedOnce } from './events-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.join(__dirname, 'fixtures', 'api')

describe('app user paths module', () => {

  describe('app cache customization', () => {
    it('by default', async () => {
      const output = await runTestApp('app-user-paths', '-default')
      const expected = path.join(app.getPath('appData'), 'app-user-paths')
      expect(output.userCache).to.not.equal(expected)
      expect(output.userData).to.not.equal(expected)
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
