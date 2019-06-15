import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, session, ipcMain } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeAllWindows } from './window-helpers';
import * as https from 'https';
import * as path from 'path';
import * as fs from 'fs';
import { EventEmitter } from 'events';

const { expect } = chai

chai.use(chaiAsPromised)
const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

describe('reporting api', () => {
  it('sends a report for a deprecation', async () => {
    const reports = new EventEmitter

    // The Reporting API only works on https with valid certs. To dodge having
    // to set up a trusted certificate, hack the validator.
    session.defaultSession.setCertificateVerifyProc((req, cb) => {
      cb(0)
    })
    const certPath = path.join(fixturesPath, 'certificates')
    const options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    }

    const server = https.createServer(options, (req, res) => {
      if (req.url === '/report') {
        let data = ''
        req.on('data', (d) => data += d.toString('utf-8'))
        req.on('end', () => {
          reports.emit('report', JSON.parse(data))
        })
      }
      res.setHeader('Report-To', JSON.stringify({
        group: 'default',
        max_age: 120,
        endpoints: [ {url: `https://localhost:${(server.address() as any).port}/report`} ],
      }))
      res.setHeader('Content-Type', 'text/html')
      // using the deprecated `webkitRequestAnimationFrame` will trigger a
      // "deprecation" report.
      res.end('<script>webkitRequestAnimationFrame(() => {})</script>')
    })
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const bw = new BrowserWindow({
      show: false,
    })
    try {
      const reportGenerated = emittedOnce(reports, 'report')
      const url = `https://localhost:${(server.address() as any).port}/a`
      await bw.loadURL(url)
      const [report] = await reportGenerated
      expect(report).to.be.an('array')
      expect(report[0].type).to.equal('deprecation')
      expect(report[0].url).to.equal(url)
      expect(report[0].body.id).to.equal('PrefixedRequestAnimationFrame')
    } finally {
      bw.destroy()
      server.close()
    }
  })
})

describe('window.postMessage', () => {
  afterEach(async () => {
    await closeAllWindows()
  })

  it('sets the source and origin correctly', async () => {
    const w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    w.loadURL(`file://${fixturesPath}/pages/window-open-postMessage-driver.html`)
    const [, message] = await emittedOnce(ipcMain, 'complete')
    expect(message.data).to.equal('testing')
    expect(message.origin).to.equal('file://')
    expect(message.sourceEqualsOpener).to.equal(true)
    expect(message.eventOrigin).to.equal('file://')
  })
})
