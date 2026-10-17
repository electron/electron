import { ipcMain, session, webContents as webContentsModule, type WebContents } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { once, on } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';

import { listen, waitUntil } from './lib/spec-helpers.ts';

// Toggle to add extra debug output
const DEBUG = !process.env.CI;

describe('ServiceWorkerMain module', () => {
  const fixtures = path.resolve(import.meta.dirname, 'fixtures');
  const preloadRealmFixtures = path.resolve(fixtures, 'api/preload-realm');
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
      serviceWorkers.on('running-status-changed', ({ versionId, runningStatus }) => {
        console.log(`version ${versionId} is ${runningStatus}`);
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
      wc.on('console-message', ({ message }) => {
        console.log(message);
      });
    }
  });

  afterEach(async () => {
    if (!wc.isDestroyed()) wc.destroy();
    server.close();
    ses.getPreloadScripts().map(({ id }) => ses.unregisterPreloadScript(id));
  });

  function registerPreload(scriptName: string) {
    const id = ses.registerPreloadScript({
      type: 'service-worker',
      filePath: path.resolve(preloadRealmFixtures, scriptName)
    });
    expect(id).toBeTypeOf('string');
  }

  async function loadWorkerScript(scriptUrl?: string) {
    const scriptParams = scriptUrl ? `?scriptUrl=${scriptUrl}` : '';
    return wc.loadURL(`${baseUrl}/index.html${scriptParams}`);
  }

  async function unregisterAllServiceWorkers() {
    await wc.executeJavaScript(
      `(${async function () {
        const registrations = await navigator.serviceWorker.getRegistrations();
        for (const registration of registrations) {
          registration.unregister();
        }
      }}())`
    );
  }

  async function waitForServiceWorker(
    expectedRunningStatus: Electron.ServiceWorkersRunningStatusChangedEventParams['runningStatus'] = 'starting'
  ) {
    const serviceWorkerPromise = new Promise<Electron.ServiceWorkerMain>((resolve) => {
      function onRunningStatusChanged({
        versionId,
        runningStatus
      }: Electron.ServiceWorkersRunningStatusChangedEventParams) {
        if (runningStatus === expectedRunningStatus) {
          const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId)!;
          serviceWorkers.off('running-status-changed', onRunningStatusChanged);
          resolve(serviceWorker);
        }
      }
      serviceWorkers.on('running-status-changed', onRunningStatusChanged);
    });
    const serviceWorker = await serviceWorkerPromise;
    expect(serviceWorker).not.toBeUndefined();
    return serviceWorker!;
  }

  // A newly registered worker is already 'running'
  // while its install event is still in flight.
  // Stopping it then fails the install, which makes the version redundant
  // and deletes the registration,
  // so tests that stop the worker must first wait until it is activated.
  // Call this after the worker reports 'running', so the registration exists.
  async function waitForActivation() {
    await wc.executeJavaScript(
      `(${async function () {
        const registration = await navigator.serviceWorker.getRegistration();
        if (!registration) throw new Error('service worker registration not found');
        const worker = registration.installing ?? registration.waiting ?? registration.active;
        if (!worker) throw new Error('service worker registration has no worker');
        while (worker.state !== 'activated') {
          if (worker.state === 'redundant') throw new Error('service worker became redundant before activating');
          await new Promise((resolve) => worker.addEventListener('statechange', resolve, { once: true }));
        }
      }}())`
    );
  }

  /** Runs a test using the framework in preload-tests.js */
  const runTest = async (serviceWorker: Electron.ServiceWorkerMain, rpc: { name: string; args: any[] }) => {
    const uuid = crypto.randomUUID();
    serviceWorker.send('test', uuid, rpc.name, ...rpc.args);
    return new Promise((resolve, reject) => {
      serviceWorker.ipc.once(`test-result-${uuid}`, (_event, { error, result }) => {
        if (error) {
          reject(result);
        } else {
          resolve(result);
        }
      });
    });
  };

  describe('serviceWorkers.getWorkerFromVersionID', () => {
    it('returns undefined for non-live service worker', () => {
      expect(serviceWorkers.getWorkerFromVersionID(-1)).toBeUndefined();
      expect(serviceWorkers._getWorkerFromVersionIDIfExists(-1)).toBeUndefined();
    });

    it('returns instance for live service worker', async () => {
      const runningStatusChanged = once(serviceWorkers, 'running-status-changed');
      loadWorkerScript();
      const [{ versionId }] = await runningStatusChanged;
      const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
      expect(serviceWorker).not.toBeUndefined();
      const ifExistsServiceWorker = serviceWorkers._getWorkerFromVersionIDIfExists(versionId);
      expect(ifExistsServiceWorker).not.toBeUndefined();
      expect(serviceWorker).toBe(ifExistsServiceWorker);
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
      expect(actualStatuses).toEqual(['starting', 'stopping']);
      expect(serviceWorker).not.toBeUndefined();
    });

    it('does not find unregistered service worker', async () => {
      loadWorkerScript();
      const runningServiceWorker = await waitForServiceWorker('running');
      const { versionId } = runningServiceWorker;
      unregisterAllServiceWorkers();
      await waitUntil(() => runningServiceWorker.isDestroyed());
      const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
      expect(serviceWorker).toBeUndefined();
    });
  });

  describe('isDestroyed()', () => {
    it('is not destroyed after being created', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker.isDestroyed()).toBe(false);
    });

    it('is destroyed after being unregistered', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker.isDestroyed()).toBe(false);
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
      await startWorkerPromise;
      const otherSW = await startWorkerPromise;
      expect(otherSW).toBe(serviceWorker);
    });

    it('rejects with starting workers', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('starting');
      const startWorkerPromise = serviceWorkers.startWorkerForScope(serviceWorker.scope);
      await expect(startWorkerPromise).rejects.toThrow();
    });

    it('starts previously stopped worker', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      await waitForActivation();
      const { scope } = serviceWorker;
      const stoppedPromise = waitForServiceWorker('stopped');
      await serviceWorkers._stopAllWorkers();
      expect(await stoppedPromise).toBe(serviceWorker);
      const startedWorker = await serviceWorkers.startWorkerForScope(scope);
      expect(startedWorker).toBe(serviceWorker);
    });

    it('resolves when called twice', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const { scope } = serviceWorker;
      const [swA, swB] = await Promise.all([
        serviceWorkers.startWorkerForScope(scope),
        serviceWorkers.startWorkerForScope(scope)
      ]);
      expect(swA).toBe(swB);
      expect(swA).toBe(serviceWorker);
    });
  });

  describe('startTask()', () => {
    it('has no tasks in-flight initially', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker._countExternalRequests()).toBe(0);
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
      expect(task).toBeTypeOf('object');
      expect(task).toHaveProperty('end', expect.any(Function));
      expect(serviceWorker._countExternalRequests()).toBe(1);

      task.end();

      // Count will decrement after Promise.finally callback
      await new Promise<void>(queueMicrotask);
      expect(serviceWorker._countExternalRequests()).toBe(0);
    });

    it('can have more than one active task', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');

      const taskA = serviceWorker.startTask();
      const taskB = serviceWorker.startTask();
      expect(serviceWorker._countExternalRequests()).toBe(2);
      taskB.end();
      taskA.end();

      // Count will decrement after Promise.finally callback
      await new Promise<void>(queueMicrotask);
      expect(serviceWorker._countExternalRequests()).toBe(0);
    });

    it('throws when starting task after destroyed', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      await unregisterAllServiceWorkers();
      await waitUntil(() => serviceWorker.isDestroyed());
      expect(() => serviceWorker.startTask()).toThrow();
    });

    it('throws when ending task after destroyed', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      const task = serviceWorker.startTask();
      await unregisterAllServiceWorkers();
      await waitUntil(() => serviceWorker.isDestroyed());
      expect(() => task.end()).toThrow();
    });
  });

  describe("'versionId' property", () => {
    it('matches the expected value', async () => {
      const runningStatusChanged = once(serviceWorkers, 'running-status-changed');
      wc.loadURL(`${baseUrl}/index.html`);
      const [{ versionId }] = await runningStatusChanged;
      const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId);
      expect(serviceWorker).not.toBeUndefined();
      if (!serviceWorker) return;
      expect(serviceWorker).toHaveProperty('versionId', expect.any(Number));
      expect(serviceWorker.versionId).toBe(versionId);
    });
  });

  describe("'scope' property", () => {
    it('matches the expected value', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker).not.toBeUndefined();
      if (!serviceWorker) return;
      expect(serviceWorker).toHaveProperty('scope', expect.any(String));
      expect(serviceWorker.scope).toBe(`${baseUrl}/`);
    });
  });

  describe("'scriptURL' property", () => {
    it('matches the expected value', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker();
      expect(serviceWorker).not.toBeUndefined();
      if (!serviceWorker) return;
      expect(serviceWorker).toHaveProperty('scriptURL', expect.any(String));
      expect(serviceWorker.scriptURL).toBe(`${baseUrl}/sw.js`);
    });
  });

  describe('ipc', () => {
    beforeEach(() => {
      registerPreload('preload-tests.js');
    });

    describe('on(channel)', () => {
      it('can receive a message during startup', async () => {
        registerPreload('preload-send-ping.js');
        loadWorkerScript();
        const serviceWorker = await waitForServiceWorker();
        const pingPromise = once(serviceWorker.ipc, 'ping');
        await pingPromise;
      });

      it('receives a message', async () => {
        loadWorkerScript();
        const serviceWorker = await waitForServiceWorker('running');
        const pingPromise = once(serviceWorker.ipc, 'ping');
        runTest(serviceWorker, { name: 'testSend', args: ['ping'] });
        await pingPromise;
      });

      it('does not receive message on ipcMain', async () => {
        loadWorkerScript();
        const serviceWorker = await waitForServiceWorker('running');
        const abortController = new AbortController();
        try {
          let pingReceived = false;
          once(ipcMain, 'ping', { signal: abortController.signal }).then(() => {
            pingReceived = true;
          });
          runTest(serviceWorker, { name: 'testSend', args: ['ping'] });
          await once(ses, '-ipc-message');
          await new Promise<void>(queueMicrotask);
          expect(pingReceived).toBe(false);
        } finally {
          abortController.abort();
        }
      });
    });

    describe('handle(channel)', () => {
      it('receives and responds to message', async () => {
        loadWorkerScript();
        const serviceWorker = await waitForServiceWorker('running');
        serviceWorker.ipc.handle('ping', () => 'pong');
        const result = await runTest(serviceWorker, { name: 'testInvoke', args: ['ping'] });
        expect(result).toBe('pong');
      });

      it('works after restarting worker', async () => {
        loadWorkerScript();
        const serviceWorker = await waitForServiceWorker('running');
        await waitForActivation();
        const { scope } = serviceWorker;
        serviceWorker.ipc.handle('ping', () => 'pong');
        await serviceWorkers._stopAllWorkers();
        // Compare identity as a boolean: `ipc` is now a read-only own property,
        // which breaks vitest's object diff when toBe() fails.
        const startedWorker = await serviceWorkers.startWorkerForScope(scope);
        expect(startedWorker === serviceWorker, 'startWorkerForScope() restarted the original worker').toBe(true);
        const result = await runTest(serviceWorker, { name: 'testInvoke', args: ['ping'] });
        expect(result).toBe('pong');
      });
    });
  });

  describe('process object', () => {
    beforeEach(() => {
      registerPreload('preload-tests.js');
    });

    it('exposes process props from the worker startup data push', async () => {
      // The service-worker preload realm receives its preload set + process
      // info via the per-process ElectronWorkerStartup mojo push at
      // RenderProcessReady — there is no sync IPC fallback. This verifies the
      // push delivered the same shape the preload realm needs.
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const result: any = await runTest(serviceWorker, { name: 'testProcess', args: [] });
      expect(result.type).toBe('service-worker');
      expect(result.sandboxed).toBe(true);
      expect(result.contextIsolated).toBe(true);
      expect(result.arch).toBe(process.arch);
      expect(result.platform).toBe(process.platform);
      expect(result.version).toBe(process.version);
      expect(result.electronVersion).toBe(process.versions.electron);
      expect(result.chromeVersion).toBe(process.versions.chrome);
      expect(result.nodeVersion).toBe(process.versions.node);
      expect(result.hasEnv, 'preload realm should have process.env').toBe(true);
      expect(result.hasExecPath, 'preload realm should have process.execPath').toBe(true);
    });
  });

  describe('contextBridge', () => {
    beforeEach(() => {
      registerPreload('preload-tests.js');
    });

    it('can evaluate func from preload realm', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const result = await runTest(serviceWorker, { name: 'testEvaluate', args: ['evalConstructorName'] });
      expect(result).toBe('ServiceWorkerGlobalScope');
    });

    it('does not leak prototypes', async () => {
      loadWorkerScript();
      const serviceWorker = await waitForServiceWorker('running');
      const result = await runTest(serviceWorker, { name: 'testPrototypeLeak', args: [] });
      expect(result).toBe(true);
    });
  });

  describe('extensions', () => {
    const extensionFixtures = path.join(fixtures, 'extensions');
    const testExtensionFixture = path.join(extensionFixtures, 'mv3-service-worker');

    beforeEach(async () => {
      ses = session.fromPartition(`persist:${crypto.randomUUID()}-service-worker-main-spec`);
      serviceWorkers = ses.serviceWorkers;
    });

    it('can observe extension service workers', async () => {
      const serviceWorkerPromise = waitForServiceWorker();
      const extension = await ses.extensions.loadExtension(testExtensionFixture);
      const serviceWorker = await serviceWorkerPromise;
      expect(serviceWorker.scope).toBe(extension.url);
    });

    it('has extension state available when preload runs', async () => {
      registerPreload('preload-send-extension.js');
      const serviceWorkerPromise = waitForServiceWorker();
      const extensionPromise = ses.extensions.loadExtension(testExtensionFixture);
      const serviceWorker = await serviceWorkerPromise;
      const result = await new Promise<any>((resolve) => {
        serviceWorker.ipc.handleOnce('preload-extension-result', (_event, result) => {
          resolve(result);
        });
      });
      const extension = await extensionPromise;
      expect(result).toBeTypeOf('object');
      expect(result.id).toBe(extension.id);
      expect(result.manifest).toEqual(result.manifest);
    });
  });
});
