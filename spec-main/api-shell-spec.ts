import { BrowserWindow, shell } from 'electron'
import { closeAllWindows } from './window-helpers'
import { emittedOnce } from './events-helpers'
import * as http from 'http'
import { AddressInfo } from 'net'

describe('shell module', () => {
  describe('shell.openExternal()', () => {
    let envVars: Record<string, string | undefined> = {}

    beforeEach(function () {
      envVars = {
        display: process.env.DISPLAY,
        de: process.env.DE,
        browser: process.env.BROWSER
      }
    })

    afterEach(async () => {
      // reset env vars to prevent side effects
      if (process.platform === 'linux') {
        process.env.DE = envVars.de
        process.env.BROWSER = envVars.browser
        process.env.DISPLAY = envVars.display
      }
    })
    afterEach(closeAllWindows)

    it('opens an external link', async () => {
      let url = 'http://127.0.0.1'
      let requestReceived
      if (process.platform === 'linux') {
        process.env.BROWSER = '/bin/true'
        process.env.DE = 'generic'
        process.env.DISPLAY = ''
        requestReceived = Promise.resolve()
      } else if (process.platform === 'darwin') {
        // On the Mac CI machines, Safari tries to ask for a password to the
        // code signing keychain we set up to test code signing (see
        // https://github.com/electron/electron/pull/19969#issuecomment-526278890),
        // so use a blur event as a crude proxy.
        const w = new BrowserWindow({ show: true })
        requestReceived = emittedOnce(w, 'blur')
      } else {
        const server = http.createServer((req, res) => {
          res.end()
        })
        await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
        requestReceived = new Promise(resolve => server.on('connection', () => resolve()))
        url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
      }

      await Promise.all([
        shell.openExternal(url),
        requestReceived
      ])
    })
  })
})
