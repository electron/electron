import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';
import { session, webContents, WebContents } from 'electron/main';
import { expect } from 'chai';
import { v4 } from 'uuid';
import { listen } from './lib/spec-helpers';
import { on, once } from 'node:events';

const partition = 'service-workers-spec';

describe('session.serviceWorkers', () => {
  let ses: Electron.Session;
  let server: http.Server;
  let baseUrl: string;
  let w: WebContents;

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
    const { port } = await listen(server);
    baseUrl = `http://localhost:${port}/${uuid}`;

    w = (webContents as typeof ElectronInternal.WebContents).create({ session: ses });
  });

  afterEach(async () => {
    w.destroy();
    server.close();
    await ses.clearStorageData();
  });

  describe('getAllRunning()', () => {
    it('should initially report none are running', () => {
      expect(ses.serviceWorkers.getAllRunning()).to.deep.equal({});
    });

    it('should report one as running once you load a page with a service worker', async () => {
      w.loadURL(`${baseUrl}/index.html`);
      await once(ses.serviceWorkers, 'console-message');
      const workers = ses.serviceWorkers.getAllRunning();
      const ids = Object.keys(workers) as any[] as number[];
      expect(ids).to.have.lengthOf(1, 'should have one worker running');
    });
  });

  describe('getFromVersionID()', () => {
    it('should report the correct script url and scope', async () => {
      w.loadURL(`${baseUrl}/index.html`);
      const eventInfo = await once(ses.serviceWorkers, 'console-message');
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
      w.loadURL(`${baseUrl}/logs.html`);
      for await (const [, details] of on(ses.serviceWorkers, 'console-message')) {
        messages[details.message] = details;
        expect(details).to.have.property('source', 'console-api');
        if (Object.keys(messages).length >= 4) break;
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
