import { session, webContents as webContentsModule, WebContents } from 'electron/main';

import { expect } from 'chai';

import { once, on } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';

import { listen, waitUntil } from './lib/spec-helpers';

// Toggle to add extra debug output
const DEBUG = !process.env.CI;

describe('ServiceWorkerMain module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');
  const webContentsInternal: typeof ElectronInternal.WebContents = webContentsModule as any;

  let ses: Electron.Session;
  let serviceWorkers: Electron.ServiceWorkers;
  let server: http.Server;
  let baseUrl: string;
  let wc: WebContents;

  beforeEach(async () => {
    ses = session.fromPartition(`service-worker-main-spec-${crypto.randomUUID()}`);
    serviceWorkers = ses.serviceWorkers;

    if (DEBUG) {
      serviceWorkers.on('console-message', (_e, details) => {
        console.log(details.message);
      });
    }

    const uuid = crypto.randomUUID();
    server = http.createServer((req, res) => {
      const url = new URL(req.url!, `http://${req.headers.host}`);
      // /{uuid}/{file}
      const file = url.pathname!.split('/')[2]!;

      if (file.endsWith('.js')) {
        res.setHeader('Content-Type', 'application/javascript');
      }
      res.end(fs.readFileSync(path.resolve(fixtures, 'api', 'service-workers', file)));
    });
    const { port } = await listen(server);
    baseUrl = `http://localhost:${port}/${uuid}`;

    wc = webContentsInternal.create({ session: ses });

    if (DEBUG) {
      wc.on('console-message', (_e, _l, message) => {
        console.log(message);
      });
    }
  });

  afterEach(async () => {
    if (!wc.isDestroyed()) wc.destroy();
    server.close();
  });

  async function loadWorkerScript (scriptUrl?: string) {
    const scriptParams = scriptUrl ? `?scriptUrl=${scriptUrl}` : '';
    return wc.loadURL(`${baseUrl}/index.html${scriptParams}`);
  }

  async function unregisterAllServiceWorkers () {
    await wc.executeJavaScript(`(${async function () {
      const registrations = await navigator.serviceWorker.getRegistrations();
      for (const registration of registrations) {
        registration.unregister();
      }
    }}())`);
  }

  async function waitForServiceWorker (expectedRunningStatus: Electron.ServiceWorkersRunningStatusChangedEventParams['runningStatus'] = 'starting') {
    const serviceWorkerPromise = new Promise<Electron.ServiceWorkerMain>((resolve) => {
      function onRunningStatusChanged ({ versionId, runningStatus }: Electron.ServiceWorkersRunningStatusChangedEventParams) {
        if (runningStatus === expectedRunningStatus) {
          const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId)!;
          serviceWorkers.off('running-status-changed', onRunningStatusChanged);
          resolve(serviceWorker);
        }
      }
      serviceWorkers.on('running-status-changed', onRunningStatusChanged);
    });
    const serviceWorker = await serviceWorkerPromise;
    expect(serviceWorker).to.not.be.undefined();
    return serviceWorker!;
  }

  describe('serviceWorkers.getWorkerFromVersionID', () => {
    it('returns undefined for non-live service worker', () => {
      expect(serviceWorkers.getWorkerFromVersionID(-1)).to.be.undefined();
      expect(serviceWorkers._getWorkerFromVersionIDIfExists(-1)).to.be.undefined();
    });

    it('returns instance for live service worker', async () => {
      const runningStatusChanged = once(serviceWorkers, 'running-status-changed');
      loadWorkerScript();
      const [{ versionId }] = await runningStatusChanged;
      const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
      expect(serviceWorker).to.not.be.undefined();
      const ifExistsServiceWorker = serviceWorkers._getWorkerFromVersionIDIfExists(versionId);
      expect(ifExistsServiceWorker).to.not.be.undefined();
      expect(serviceWorker).to.equal(ifExistsServiceWorker);
    });

    it('does not crash on script error', async () => {
      wc.loadURL(`${baseUrl}/index.html?scriptUrl=sw-script-error.js`);
      let serviceWorker;
      const actualStatuses = [];
      for await (const [{ versionId, runningStatus }] of on(serviceWorkers, 'running-status-changed')) {
        if (!serviceWorker) {
          serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
        }
        actualStatuses.push(runningStatus);
        if (runningStatus === 'stopping') {
          break;
        }
      }
      expect(actualStatuses).to.deep.equal(['starting', 'stopping']);
      expect(serviceWorker).to.not.be.undefined();
    });

    it('does not find unregistered service worker', async () => {
      loadWorkerScript();
      const runningServiceWorker = await waitForServiceWorker('running');
      const { versionId } = runningServiceWorker;
      unregisterAllServiceWorkers();
      await waitUntil(() => runningServiceWorker.isDestroyed());
      const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
      expect(serviceWorker).to.be.undefined();
    });
  });

  describe('isDestroyed()', () => {
    it('is not destroyed after being created', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker.isDestroyed()).to.be.false();
    });

    it('is destroyed after being unregistered', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker.isDestroyed()).to.be.false();
      await unregisterAllServiceWorkers();
      await waitUntil(() => serviceWorker.isDestroyed());
    });
  });

  describe('"running-status-changed" event', () => {
    it('handles when content::ServiceWorkerVersion has been destroyed', async () => {
      loadWorkerScript('sw-unregister-self.js');
      const serviceWorker = await waitForServiceWorker('running');
      await waitUntil(() => serviceWorker.isDestroyed());
    });
  });

  describe('startWorkerForScope()', () => {
    it('resolves with running workers', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const startWorkerPromise = serviceWorkers.startWorkerForScope(serviceWorker.scope);
      await expect(startWorkerPromise).to.eventually.be.fulfilled();
      const otherSW = await startWorkerPromise;
      expect(otherSW).to.equal(serviceWorker);
    });

    it('rejects with starting workers', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('starting');
      const startWorkerPromise = serviceWorkers.startWorkerForScope(serviceWorker.scope);
      await expect(startWorkerPromise).to.eventually.be.rejected();
    });

    it('starts previously stopped worker', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const { scope } = serviceWorker;
      const stoppedPromise = waitForServiceWorker('stopped');
      await serviceWorkers._stopAllWorkers();
      await stoppedPromise;
      const startWorkerPromise = serviceWorkers.startWorkerForScope(scope);
      await expect(startWorkerPromise).to.eventually.be.fulfilled();
    });

    it('resolves when called twice', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const { scope } = serviceWorker;
      const [swA, swB] = await Promise.all([
        serviceWorkers.startWorkerForScope(scope),
        serviceWorkers.startWorkerForScope(scope)
      ]);
      expect(swA).to.equal(swB);
      expect(swA).to.equal(serviceWorker);
    });
  });

  describe('startTask()', () => {
    it('has no tasks in-flight initially', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker._countExternalRequests()).to.equal(0);
    });

    it('can start and end a task', async () => {
      loadWorkerScript();

      // Internally, ServiceWorkerVersion buckets tasks into requests made
      // during and after startup.
      // ServiceWorkerContext::CountExternalRequestsForTest only considers
      // requests made while SW is in running status so we need to wait for that
      // to read an accurate count.
      const serviceWorker = await waitForServiceWorker('running');

      const task = serviceWorker.startTask();
      expect(task).to.be.an('object');
      expect(task).to.have.property('end').that.is.a('function');
      expect(serviceWorker._countExternalRequests()).to.equal(1);

      task.end();

      // Count will decrement after Promise.finally callback
      await new Promise<void>(queueMicrotask);
      expect(serviceWorker._countExternalRequests()).to.equal(0);
    });

    it('can have more than one active task', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');

      const taskA = serviceWorker.startTask();
      const taskB = serviceWorker.startTask();
      expect(serviceWorker._countExternalRequests()).to.equal(2);
      taskB.end();
      taskA.end();

      // Count will decrement after Promise.finally callback
      await new Promise<void>(queueMicrotask);
      expect(serviceWorker._countExternalRequests()).to.equal(0);
    });

    it('throws when starting task after destroyed', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      await unregisterAllServiceWorkers();
      await waitUntil(() => serviceWorker.isDestroyed());
      expect(() => serviceWorker.startTask()).to.throw();
    });

    it('throws when ending task after destroyed', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      const task = serviceWorker.startTask();
      await unregisterAllServiceWorkers();
      await waitUntil(() => serviceWorker.isDestroyed());
      expect(() => task.end()).to.throw();
    });
  });

  describe("'versionId' property", () => {
    it('matches the expected value', async () => {
      const runningStatusChanged = once(serviceWorkers, 'running-status-changed');
      wc.loadURL(`${baseUrl}/index.html`);
      const [{ versionId }] = await runningStatusChanged;
      const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
      expect(serviceWorker).to.not.be.undefined();
      if (!serviceWorker) return;
      expect(serviceWorker).to.have.property('versionId').that.is.a('number');
      expect(serviceWorker.versionId).to.equal(versionId);
    });
  });

  describe("'scope' property", () => {
    it('matches the expected value', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker).to.not.be.undefined();
      if (!serviceWorker) return;
      expect(serviceWorker).to.have.property('scope').that.is.a('string');
      expect(serviceWorker.scope).to.equal(`${baseUrl}/`);
    });
  });
});
