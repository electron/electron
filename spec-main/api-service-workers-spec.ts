import * as fs from 'fs';
import * as http from 'http';
import * as path from 'path';
import { session, webContents, WebContents } from 'electron/main';
import { expect } from 'chai';
import { v4 } from 'uuid';
import { AddressInfo } from 'net';
import { emittedOnce, emittedNTimes } from './events-helpers';

const partition = 'service-workers-spec';

describe('session.serviceWorkers', () => {
  let ses: Electron.Session;
  let server: http.Server;
  let baseUrl: string;
  let w: WebContents = null as unknown as WebContents;

  before(async () => {
    ses = session.fromPartition(partition);
    await ses.clearStorageData();
  });

  beforeEach(async () => {
    const uuid = v4();

    server = http.createServer((req, res) => {
      // /{uuid}/{file}
      const file = req.url!.split('/')[2]!;

      if (file.endsWith('.js')) {
        res.setHeader('Content-Type', 'application/javascript');
      }
      res.end(fs.readFileSync(path.resolve(__dirname, 'fixtures', 'api', 'service-workers', file)));
    });
    await new Promise<void>(resolve => {
      server.listen(0, '127.0.0.1', () => {
        baseUrl = `http://localhost:${(server.address() as AddressInfo).port}/${uuid}`;
        resolve();
      });
    });

    w = (webContents as any).create({ session: ses });
  });

  afterEach(async () => {
    (w as any).destroy();
    server.close();
    await ses.clearStorageData();
  });

  describe('getAllRunning()', () => {
    it('should initially report none are running', () => {
      expect(ses.serviceWorkers.getAllRunning()).to.deep.equal({});
    });

    it('should report one as running once you load a page with a service worker', async () => {
      await emittedOnce(ses.serviceWorkers, 'console-message', () => w.loadURL(`${baseUrl}/index.html`));
      const workers = ses.serviceWorkers.getAllRunning();
      const ids = Object.keys(workers) as any[] as number[];
      expect(ids).to.have.lengthOf(1, 'should have one worker running');
    });
  });

  describe('getFromVersionID()', () => {
    it('should report the correct script url and scope', async () => {
      const eventInfo = await emittedOnce(ses.serviceWorkers, 'console-message', () => w.loadURL(`${baseUrl}/index.html`));
      const details: Electron.MessageDetails = eventInfo[1];
      const worker = ses.serviceWorkers.getFromVersionID(details.versionId);
      expect(worker).to.not.equal(null);
      expect(worker).to.have.property('scope', baseUrl + '/');
      expect(worker).to.have.property('scriptUrl', baseUrl + '/sw.js');
    });
  });

  describe('console-message event', () => {
    it('should correctly keep the source, message and level', async () => {
      const messages: Record<string, Electron.MessageDetails> = {};
      const events = await emittedNTimes(ses.serviceWorkers, 'console-message', 4, () => w.loadURL(`${baseUrl}/logs.html`));
      for (const event of events) {
        messages[event[1].message] = event[1];

        expect(event[1]).to.have.property('source', 'console-api');
      }

      expect(messages).to.have.property('log log');
      expect(messages).to.have.property('info log');
      expect(messages).to.have.property('warn log');
      expect(messages).to.have.property('error log');
      expect(messages['log log']).to.have.property('level', 1);
      expect(messages['info log']).to.have.property('level', 1);
      expect(messages['warn log']).to.have.property('level', 2);
      expect(messages['error log']).to.have.property('level', 3);
    });
  });
});
