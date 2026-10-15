import { session, webContents, type WebContents } from 'electron/main';

import { v4 } from 'uuid';
import { afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { on, once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';

import { listen } from './lib/spec-helpers.ts';

const partition = 'service-workers-spec';

describe('session.serviceWorkers', () => {
  let ses: Electron.Session;
  let server: http.Server;
  let baseUrl: string;
  let w: WebContents;

  beforeAll(async () => {
    ses = session.fromPartition(partition);
    await ses.clearStorageData();
  });

  beforeEach(async () => {
    const uuid = v4();

    server = http.createServer((req, res) => {
      const url = new URL(req.url!, `http://${req.headers.host}`);
      // /{uuid}/{file}
      const file = url.pathname!.split('/')[2]!;

      if (file.endsWith('.js')) {
        res.setHeader('Content-Type', 'application/javascript');
      }
      res.end(fs.readFileSync(path.resolve(import.meta.dirname, 'fixtures', 'api', 'service-workers', file)));
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
      expect(ses.serviceWorkers.getAllRunning()).toEqual({});
    });

    it('should report one as running once you load a page with a service worker', async () => {
      w.loadURL(`${baseUrl}/index.html`);
      await once(ses.serviceWorkers, 'console-message');
      const workers = ses.serviceWorkers.getAllRunning();
      const ids = Object.keys(workers) as any[] as number[];
      expect(ids, 'should have one worker running').toHaveLength(1);
    });
  });

  describe('getFromVersionID()', () => {
    it('should report the correct script url and scope', async () => {
      w.loadURL(`${baseUrl}/index.html`);
      const eventInfo = await once(ses.serviceWorkers, 'console-message');
      const details: Electron.MessageDetails = eventInfo[1];
      const worker = ses.serviceWorkers.getFromVersionID(details.versionId);
      expect(worker).not.toBe(null);
      expect(worker).toHaveProperty('scope', baseUrl + '/');
      expect(worker).toHaveProperty('scriptUrl', baseUrl + '/sw.js');
    });
  });

  describe('console-message event', () => {
    it('should correctly keep the source, message and level', async () => {
      const messages: Record<string, Electron.MessageDetails> = {};
      w.loadURL(`${baseUrl}/index.html?scriptUrl=sw-logs.js`);
      for await (const [, details] of on(ses.serviceWorkers, 'console-message')) {
        messages[details.message] = details;
        expect(details).toHaveProperty('source', 'console-api');
        if (Object.keys(messages).length >= 4) break;
      }

      expect(messages).toHaveProperty('log log');
      expect(messages).toHaveProperty('info log');
      expect(messages).toHaveProperty('warn log');
      expect(messages).toHaveProperty('error log');
      expect(messages['log log']).toHaveProperty('level', 1);
      expect(messages['info log']).toHaveProperty('level', 1);
      expect(messages['warn log']).toHaveProperty('level', 2);
      expect(messages['error log']).toHaveProperty('level', 3);
    });
  });
});
