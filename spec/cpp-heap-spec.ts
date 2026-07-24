import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';

import { ifdescribe, isTestingBindingAvailable, itremote, startRemoteControlApp } from './lib/spec-helpers';

describe('cpp heap', () => {
  describe('app module', () => {
    it('should not allocate on every require', async () => {
      const { remotely } = await startRemoteControlApp();
      const [usedBefore, usedAfter] = await remotely(async () => {
        const { getCppHeapStatistics } = require('node:v8');
        const heapStatsBefore = getCppHeapStatistics('brief');
        {
          const { app } = require('electron');
          console.log(app.name);
        }
        {
          const { app } = require('electron');
          console.log(app.dock);
        }
        const heapStatsAfter = getCppHeapStatistics('brief');
        return [heapStatsBefore.used_size_bytes, heapStatsAfter.used_size_bytes];
      });
      expect(usedBefore).to.be.equal(usedAfter);
    });

    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const state = recordState();
          return containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / App']);
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true);
    });
  });

  describe('session module', () => {
    it('does not crash on exit with live session wrappers', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(async () => {
        const { app, session } = require('electron');

        const sessions = [
          session.defaultSession,
          session.fromPartition('cppheap-exit'),
          session.fromPartition('persist:cppheap-exit-persist')
        ];

        // We want to test GC on shutdown, so add a global reference
        // to these sessions to prevent pre-shutdown GC.
        (globalThis as any).sessionRefs = sessions;

        // We want to test CppGC-traced references during shutdown.
        // The CppGC-managed cookies will do that; but since they're
        // lazy-created, access them here to ensure they're live.
        sessions.forEach((ses) => ses.cookies);

        setTimeout(() => app.quit());
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
    });

    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { session, BrowserWindow } = require('electron');
          const { once } = require('node:events');
          const assert = require('node:assert');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const session1 = session.defaultSession;
          console.log(session1.getStoragePath());
          const session2 = session.fromPartition('cppheap1');
          const session3 = session.fromPartition('cppheap1');
          const session4 = session.fromPartition('cppheap2');
          console.log(session2.cookies);
          assert.strictEqual(session2, session3);
          assert.notStrictEqual(session2, session4);
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              session: session.fromPartition('cppheap1')
            }
          });
          await w.loadURL('about:blank');
          const state = recordState();
          const isClosed = once(w, 'closed');
          w.destroy();
          await isClosed;
          const numSessions = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / Session'], {
            occurrences: 4
          });
          const canTraceJSReferences = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / Session',
            'Electron / Cookies'
          ]);
          return numSessions && canTraceJSReferences;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true);
    });
  });

  describe('cookies module', () => {
    it('should return the same Cookies instance for the same session', async () => {
      const { remotely } = await startRemoteControlApp();
      const result = await remotely(async () => {
        const { session } = require('electron');
        const ses = session.fromPartition('cookies-identity');
        const cookies1 = ses.cookies;
        const cookies2 = ses.cookies;
        return cookies1 === cookies2;
      });
      expect(result).to.equal(true, 'cookies getter should return the same instance');
    });

    it('should survive GC when only referenced through session', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { session } = require('electron');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          // Access cookies to create the C++ object, then drop the JS reference.
          const ses = session.defaultSession;
          let cookies: any = ses.cookies;
          await cookies.get({});
          cookies = null;

          // Force GC — the Cookies object should survive because
          // Session traces it via cppgc::Member.
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const stillAlive = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / Session',
            'Electron / Cookies'
          ]);
          return stillAlive;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true, 'Cookies should survive GC when traced from Session');
    });
  });

  describe('downloadItem module', () => {
    it('should survive GC while a download is in progress', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { session } = require('electron');
          const http = require('node:http');
          const os = require('node:os');
          const path = require('node:path');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          // Server that sends headers + part of the body, then holds the
          // connection open so the download stays in the IN_PROGRESS state.
          let releaseBody: () => void = () => {};
          const bodyHeld = new Promise<void>((resolve) => {
            releaseBody = resolve;
          });
          const server = http.createServer(async (_req: any, res: any) => {
            res.writeHead(200, {
              'Content-Length': 1024,
              'Content-Type': 'application/octet-stream',
              'Content-Disposition': 'attachment; filename="inflight.bin"'
            });
            res.write(Buffer.alloc(512));
            await bodyHeld;
            res.end(Buffer.alloc(512));
          });
          await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
          const { port } = server.address();

          const ses = session.fromPartition('cppheap-download-inflight');
          let item: any = await new Promise((resolve) => {
            ses.once('will-download', (_e: any, downloadItem: any) => resolve(downloadItem));
            ses.downloadURL(`http://127.0.0.1:${port}`);
          });
          item.savePath = path.join(os.tmpdir(), 'cppheap-download-inflight.bin');

          // Drop the only JS reference while the download is still running.
          item = null;

          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const stillAlive = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / DownloadItem']);

          releaseBody();
          server.close();
          return stillAlive;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true, 'DownloadItem should survive GC while the download is in progress');
    });

    it('should be released after the download completes', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { session } = require('electron');
          const http = require('node:http');
          const os = require('node:os');
          const path = require('node:path');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          const payload = Buffer.alloc(1024);
          const server = http.createServer((_req: any, res: any) => {
            res.writeHead(200, {
              'Content-Length': payload.length,
              'Content-Type': 'application/octet-stream',
              'Content-Disposition': 'attachment; filename="done.bin"'
            });
            res.end(payload);
          });
          await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
          const { port } = server.address();

          const ses = session.fromPartition('cppheap-download-done');
          await new Promise<void>((resolve) => {
            ses.once('will-download', (_e: any, item: any) => {
              item.savePath = path.join(os.tmpdir(), 'cppheap-download-done.bin');
              item.once('done', () => resolve());
            });
            ses.downloadURL(`http://127.0.0.1:${port}`);
          });
          server.close();

          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const found = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / DownloadItem']);
          return !found;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true, 'DownloadItem should be released after the download completes and GC runs');
    });

    it('should not leak when performing multiple downloads', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(async () => {
        const { session } = require('electron');
        const http = require('node:http');
        const os = require('node:os');
        const path = require('node:path');
        const { getCppHeapStatistics } = require('node:v8');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const payload = Buffer.alloc(1024);
        const server = http.createServer((_req: any, res: any) => {
          res.writeHead(200, {
            'Content-Length': payload.length,
            'Content-Type': 'application/octet-stream',
            'Content-Disposition': 'attachment; filename="leak.bin"'
          });
          res.end(payload);
        });
        await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
        const { port } = server.address();

        const ses = session.fromPartition('cppheap-download-leak');
        const savePath = path.join(os.tmpdir(), 'cppheap-download-leak.bin');

        async function downloadOnce() {
          await new Promise<void>((resolve) => {
            ses.once('will-download', (_e: any, item: any) => {
              item.savePath = savePath;
              item.once('done', () => resolve());
            });
            ses.downloadURL(`http://127.0.0.1:${port}`);
          });
        }

        async function measure(n: number) {
          for (let i = 0; i < n; i++) {
            await downloadOnce();
          }
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          return getCppHeapStatistics('brief').used_size_bytes;
        }

        await measure(5);
        const after1 = await measure(10);
        const after2 = await measure(10);
        server.close();
        return { after1, after2 };
      });

      const growth = result.after2 - result.after1;
      expect(growth).to.be.at.most(
        result.after1 * 0.1,
        `C++ heap grew by ${growth} bytes between two identical rounds of downloads — likely a leak`
      );
    });
  });

  describe('serviceWorkerMain module', () => {
    const setupWorkerSource = `async function setupWorker(fixturesDir) {
      const { app, session, BrowserWindow } = require('electron');
      const http = require('node:http');
      const fs = require('node:fs');
      const path = require('node:path');
      const crypto = require('node:crypto');

      app.on('window-all-closed', () => {});

      const uuid = crypto.randomUUID();
      const server = http.createServer((req, res) => {
        const url = new URL(req.url, 'http://' + req.headers.host);
        const file = url.pathname.split('/')[2];
        if (file.endsWith('.js')) res.setHeader('Content-Type', 'application/javascript');
        res.end(fs.readFileSync(path.resolve(fixturesDir, file)));
      });
      await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
      const { port } = server.address();
      const baseUrl = 'http://localhost:' + port + '/' + uuid;

      const ses = session.fromPartition('cppheap-sw-' + uuid);
      const serviceWorkers = ses.serviceWorkers;
      const win = new BrowserWindow({ show: false, webPreferences: { session: ses } });

      const versionId = await new Promise((resolve) => {
        const onChange = (event) => {
          serviceWorkers.off('running-status-changed', onChange);
          resolve(event.versionId);
        };
        serviceWorkers.on('running-status-changed', onChange);
        win.webContents.loadURL(baseUrl + '/index.html');
      });

      return { win, server, ses, serviceWorkers, versionId };
    }`;

    it('does not crash on exit with a live service worker wrapper', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(
        async (fixturesDir: string, setupWorker: string) => {
          const { app } = require('electron');
          // eslint-disable-next-line no-eval
          const setup = eval('(' + setupWorker + ')');
          const ctx = await setup(fixturesDir);

          (globalThis as any).swRef = ctx.serviceWorkers.getWorkerFromVersionID(ctx.versionId);

          setTimeout(() => app.quit());
        },
        path.join(__dirname, 'fixtures', 'api', 'service-workers'),
        setupWorkerSource
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
    });

    it('should be rooted while the service worker is live', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (fixturesDir: string, setupWorker: string, heap: string, snapshotHelper: string) => {
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
          // eslint-disable-next-line no-eval
          const setup = eval('(' + setupWorker + ')');
          const ctx = await setup(fixturesDir);

          ctx.serviceWorkers.getWorkerFromVersionID(ctx.versionId);

          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const rooted = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / ServiceWorkerMain'
          ]);

          ctx.win.destroy();
          ctx.server.close();
          return rooted;
        },
        path.join(__dirname, 'fixtures', 'api', 'service-workers'),
        setupWorkerSource,
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true, 'ServiceWorkerMain should be rooted via SelfKeepAlive while the version is live');
    });

    it('should be released after the service worker is unregistered', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (fixturesDir: string, setupWorker: string, heap: string, snapshotHelper: string) => {
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
          // eslint-disable-next-line no-eval
          const setup = eval('(' + setupWorker + ')');
          const ctx = await setup(fixturesDir);

          let worker: any = ctx.serviceWorkers.getWorkerFromVersionID(ctx.versionId);
          const { versionId } = ctx;

          await ctx.win.webContents.executeJavaScript(
            '(async () => { const rs = await navigator.serviceWorker.getRegistrations(); for (const r of rs) await r.unregister(); })()'
          );

          // Wait until the wrapper has been destroyed.
          for (let i = 0; i < 100; i++) {
            if (worker && worker.isDestroyed()) break;
            await new Promise((resolve) => setTimeout(resolve, 50));
          }

          // Drop the JS reference and GC. Neither the SelfKeepAlive root nor a
          // JS wrapper should keep it alive anymore.
          worker = null;
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const rooted = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / ServiceWorkerMain'
          ]);
          const stillExists = ctx.serviceWorkers._getWorkerFromVersionIDIfExists(versionId) != null;

          ctx.win.destroy();
          ctx.server.close();
          return !rooted && !stillExists;
        },
        path.join(__dirname, 'fixtures', 'api', 'service-workers'),
        setupWorkerSource,
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(
        true,
        'ServiceWorkerMain should be released after the service worker is unregistered and GC runs'
      );
    });
  });

  describe('nativeImage module', () => {
    it('should record as node in heap snapshot while a JS reference is held', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { nativeImage } = require('electron');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          (globalThis as any).nativeImageRef = nativeImage.createEmpty();
          const state = recordState();
          const present = containsRetainingPath(state.snapshot, ['Electron / NativeImage']);
          const isPersistentRooted = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / NativeImage'
          ]);
          return present && !isPersistentRooted;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true);
    });

    it('should be released after GC when no JS references remain', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const released = await remotely(async () => {
        const { nativeImage } = require('electron');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let image: any = nativeImage.createEmpty();
        const weakRef = new WeakRef(image);
        image = null;

        return waitForGC(() => weakRef.deref() === undefined);
      });
      expect(released).to.equal(true, 'NativeImage should be released after GC when no JS references remain');
    });
  });

  describe('url loader module', () => {
    it('should not leak when performing chunked (streaming) uploads', async () => {
      const rc = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await rc.remotely(async () => {
        const { net, app } = require('electron');
        const http = require('node:http');
        const { getCppHeapStatistics } = require('node:v8');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
        const server = http.createServer((req: any, res: any) => {
          req.on('data', () => {});
          req.on('end', () => res.end('ok'));
        });
        await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
        const { port } = server.address();

        function uploadOnce() {
          return new Promise<void>((resolve, reject) => {
            const request = net.request({
              method: 'POST',
              url: `http://127.0.0.1:${port}`
            });
            request.chunkedEncoding = true;
            request.on('response', (response: any) => {
              response.on('data', () => {});
              response.on('end', () => resolve());
            });
            request.on('error', reject);
            for (let i = 0; i < 4; i++) {
              request.write(Buffer.alloc(256));
            }
            request.end();
          });
        }

        async function measure(n: number) {
          for (let i = 0; i < n; i++) {
            await uploadOnce();
          }
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          return getCppHeapStatistics('brief').used_size_bytes;
        }

        await measure(5);
        const after1 = await measure(5);
        const after2 = await measure(5);
        server.close();
        setTimeout(() => app.quit());
        return { after1, after2 };
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);

      const growth = result.after2 - result.after1;
      expect(growth).to.be.at.most(
        result.after1 * 0.1,
        `C++ heap grew by ${growth} bytes between two identical rounds of chunked uploads — likely a leak`
      );
    });

    it('completes request after JS drops reference but listeners are registered', async () => {
      const rc = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await rc.remotely(async () => {
        const { net, app } = require('electron');
        const http = require('node:http');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const server = http.createServer((req: any, res: any) => {
          res.writeHead(200);
          res.end('ok');
        });
        await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
        const { port } = server.address();

        let completed = false;
        let errorOccurred: string | null = null;
        let requestWeakRef: WeakRef<any>;

        const done = new Promise<void>((resolve) => {
          let request: any = net.request({ url: `http://127.0.0.1:${port}` });
          requestWeakRef = new WeakRef(request);

          request.on('response', (response: any) => {
            response.on('data', () => {});
            response.on('end', () => {
              completed = true;
              resolve();
            });
          });
          request.on('error', (err: Error) => {
            errorOccurred = err.message;
            resolve();
          });
          request.end();

          // Drop the JS reference immediately after registering listeners.
          request = null;
        });

        // Force GC while request is in flight.
        for (let i = 0; i < 5; i++) {
          await new Promise((resolve) => setTimeout(resolve, 10));
          v8Util.requestGarbageCollectionForTesting();
        }

        await done;
        server.close();

        for (let i = 0; i < 10; i++) {
          await new Promise((resolve) => setTimeout(resolve, 0));
          v8Util.requestGarbageCollectionForTesting();
        }
        const released = requestWeakRef!.deref() === undefined;

        setTimeout(() => app.quit());
        return { completed, errorOccurred, released };
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);

      expect(result.errorOccurred).to.equal(null, `request errored: ${result.errorOccurred}`);
      expect(result.completed).to.equal(true, 'request should complete even after JS drops reference');
      expect(result.released).to.equal(true, 'request should be released after completion');
    });

    it('keeps a ChunkedDataPipeReadableStream alive while a read is pending and releases it after', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string, snapshotHelper: string) => {
          const { protocol, net, app } = require('electron');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          const gc = async () => {
            for (let i = 0; i < 10; i++) {
              await new Promise((resolve) => setTimeout(resolve, 0));
              v8Util.requestGarbageCollectionForTesting();
            }
          };

          // Yield to the event loop and run GC until `predicate` becomes true.
          // This waits on the actual condition instead of a fixed delay,
          // returning false only if it never settles within a generous attempt
          // budget. Yielding before collecting lets any pending teardown tasks
          // run so the collection sees no lingering references.
          const gcUntil = async (predicate: () => boolean, attempts = 50) => {
            for (let i = 0; i < attempts; i++) {
              await new Promise((resolve) => setTimeout(resolve, 0));
              v8Util.requestGarbageCollectionForTesting();
              if (predicate()) return true;
            }
            return false;
          };

          let streamWeakRef: WeakRef<any> | undefined;
          let pendingRead: Promise<number> | undefined;
          let resolveReached: () => void;
          let rejectReached: (err: Error) => void;
          const reached = new Promise<void>((resolve, reject) => {
            resolveReached = resolve;
            rejectReached = reject;
          });

          protocol.interceptStreamProtocol('http', (request: any, callback: any) => {
            (async () => {
              try {
                const elements = request.uploadData || [];
                const streamElement = elements.find((e: any) => e.type === 'stream');
                if (!streamElement) {
                  callback({ statusCode: 400, data: null });
                  rejectReached(new Error('request had no stream upload element'));
                  return;
                }
                let body: any = streamElement.body;
                streamWeakRef = new WeakRef(body);
                // Drain whatever data is immediately available, then keep a read
                // that parks in net::ERR_IO_PENDING.
                const firstRead = body.read(new Uint8Array(1024));
                const drained = await Promise.race([
                  firstRead.then(
                    () => true,
                    () => true
                  ),
                  new Promise<boolean>((resolve) => setTimeout(() => resolve(false), 500))
                ]);
                pendingRead = drained ? body.read(new Uint8Array(1024)) : firstRead;
                // Drop the only JS reference to the stream while the read is in
                // flight.
                body = null;
                resolveReached();
              } catch (err) {
                rejectReached(err as Error);
              }
            })();
          });

          const request = net.request({
            method: 'POST',
            url: 'http://pending-read-host'
          });
          request.chunkedEncoding = true;
          request.on('error', () => {});
          // Write one chunk to start streaming the body, but never end the
          // request so a subsequent read has no data and stays pending.
          request.write(Buffer.from('hello'));

          try {
            // Wait until the handler has parked a pending read and dropped its
            // only JS reference to the stream.
            await reached;

            await gc();
            const state = recordState();
            const aliveWhilePending = containsRetainingPath(state.snapshot, [
              'C++ Persistent roots',
              'Electron / ChunkedDataPipeReadableStream'
            ]);
            const refAliveWhilePending = streamWeakRef!.deref() !== undefined;
            // Settle the pending read by ending the request body, then wait for
            // the read itself to complete.
            request.end();
            await pendingRead!.catch(() => {});

            // Tear down the request so the network stack drops its reference to
            // the upload stream; otherwise the live request keeps it reachable
            // and it can never be collected. The handler never produced a
            // response, so aborting is also the cleanup path.
            request.abort();

            // Wait until the stream is actually released now that no read is
            // pending and the request has been torn down.
            const released = await gcUntil(() => streamWeakRef!.deref() === undefined);

            return { ok: true, aliveWhilePending, refAliveWhilePending, released };
          } catch (err) {
            request.abort();
            return { ok: false, error: String(err) };
          } finally {
            protocol.uninterceptProtocol('http');
            setTimeout(() => app.quit());
          }
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);

      expect(result.ok, `test setup failed: ${result.error}`).to.equal(true);
      expect(result.refAliveWhilePending).to.equal(true, 'stream should still be alive while a read is pending');
      expect(result.aliveWhilePending).to.equal(
        true,
        'ChunkedDataPipeReadableStream should be rooted while a read is pending'
      );
      expect(result.released).to.equal(
        true,
        'ChunkedDataPipeReadableStream should be released after the pending read settles'
      );
    });
  });

  ifdescribe(isTestingBindingAvailable())('SafeV8Function callback conversion', () => {
    const gcTestArgv = ['--js-flags=--expose-gc'];

    it('retains repeating callback while held, allows multiple invocations, then releases', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callCount = 0;
        let repeating: any = () => {
          callCount++;
        };
        const repeatingWeakRef = new WeakRef(repeating);
        testingBinding.holdRepeatingCallbackForTesting(repeating);
        repeating = null;

        const invoked0 = testingBinding.invokeHeldRepeatingCallbackForTesting();
        const invoked1 = testingBinding.invokeHeldRepeatingCallbackForTesting();
        const invoked2 = testingBinding.invokeHeldRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => repeatingWeakRef.deref() === undefined);

        return { invoked0, invoked1, invoked2, callCount, releasedAfterClear };
      });

      expect(result.invoked0).to.equal(true, 'first invocation should succeed');
      expect(result.invoked1).to.equal(true, 'second invocation should succeed');
      expect(result.invoked2).to.equal(true, 'third invocation should succeed');
      expect(result.callCount).to.equal(3, 'callback should have been called 3 times');
      expect(result.releasedAfterClear).to.equal(true, 'callback should be released after clear');
    });

    it('consumes once callback on first invoke and releases it', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callCount = 0;
        let once: any = () => {
          callCount++;
        };
        const onceWeakRef = new WeakRef(once);
        testingBinding.holdOnceCallbackForTesting(once);
        once = null;

        const first = testingBinding.invokeHeldOnceCallbackForTesting();
        const second = testingBinding.invokeHeldOnceCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const released = await waitForGC(() => onceWeakRef.deref() === undefined);

        return { first, second, callCount, released };
      });

      expect(result.first).to.equal(true, 'first invoke should succeed');
      expect(result.second).to.equal(false, 'second invoke should fail (consumed)');
      expect(result.callCount).to.equal(1, 'callback should have been called once');
      expect(result.released).to.equal(true, 'callback should be released after consume + clear');
    });

    it('releases replaced repeating callback while keeping latest callback alive', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let callbackA: any = () => {};
        const weakA = new WeakRef(callbackA);
        testingBinding.holdRepeatingCallbackForTesting(callbackA);
        callbackA = null;

        let callbackB: any = () => {};
        const weakB = new WeakRef(callbackB);
        testingBinding.holdRepeatingCallbackForTesting(callbackB);
        callbackB = null;

        const releasedA = await waitForGC(() => weakA.deref() === undefined);

        testingBinding.clearHeldCallbacksForTesting();
        const releasedB = await waitForGC(() => weakB.deref() === undefined);

        return { releasedA, releasedB };
      });

      expect(result.releasedA).to.equal(true, 'replaced callback A should be released');
      expect(result.releasedB).to.equal(true, 'callback B should be released after clear');
    });

    it('keeps callback alive while copied holder exists and releases after all copies clear', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let repeating: any = () => {};
        const weakRef = new WeakRef(repeating);
        testingBinding.holdRepeatingCallbackForTesting(repeating);
        repeating = null;

        const copied = testingBinding.copyHeldRepeatingCallbackForTesting();
        const countAfterCopy = testingBinding.getHeldRepeatingCallbackCountForTesting();
        testingBinding.clearPrimaryHeldRepeatingCallbackForTesting();

        const invokedViaCopy = testingBinding.invokeCopiedRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => weakRef.deref() === undefined);

        return { copied, countAfterCopy, invokedViaCopy, releasedAfterClear };
      });

      expect(result.copied).to.equal(true, 'copy should succeed');
      expect(result.countAfterCopy).to.equal(2, 'should have 2 holders after copy');
      expect(result.invokedViaCopy).to.equal(true, 'invoke via copy should succeed');
      expect(result.releasedAfterClear).to.equal(true, 'callback should be released after all copies clear');
    });

    it('does not leak repeating callback when callback throws during invocation', async () => {
      const { remotely } = await startRemoteControlApp(gcTestArgv);
      const result = await remotely(async () => {
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

        const waitForGC = async (fn: () => boolean) => {
          for (let i = 0; i < 30; ++i) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
            if (fn()) return true;
          }
          return false;
        };

        let throwing: any = () => {
          throw new Error('expected test throw');
        };
        const weakRef = new WeakRef(throwing);
        testingBinding.holdRepeatingCallbackForTesting(throwing);
        throwing = null;

        const invokeResult = testingBinding.invokeHeldRepeatingCallbackForTesting();

        testingBinding.clearHeldCallbacksForTesting();
        const releasedAfterClear = await waitForGC(() => weakRef.deref() === undefined);

        return { invokeResult, releasedAfterClear };
      });

      expect(result.invokeResult).to.equal(false, 'invoke should fail (callback throws)');
      expect(result.releasedAfterClear).to.equal(true, 'throwing callback should be released after clear');
    });
  });

  describe('menu module', () => {
    // Regression test for https://github.com/electron/electron/issues/50791
    it('should not leak when rebuilding application menu', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(async () => {
        const { Menu } = require('electron');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
        const { getCppHeapStatistics } = require('node:v8');

        function buildLargeMenu() {
          return Menu.buildFromTemplate(
            Array.from({ length: 10 }, (_, i) => ({
              label: `Menu ${i}`,
              submenu: Array.from({ length: 20 }, (_, j) => ({
                label: `Item ${i}-${j}`,
                click: () => {}
              }))
            }))
          );
        }

        async function rebuildAndMeasure(n: number) {
          for (let i = 0; i < n; i++) {
            Menu.setApplicationMenu(buildLargeMenu());
          }
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          return getCppHeapStatistics('brief').used_size_bytes;
        }

        await rebuildAndMeasure(50);
        const after1 = await rebuildAndMeasure(50);
        const after2 = await rebuildAndMeasure(50);
        return { after1, after2 };
      });

      const growth = result.after2 - result.after1;
      expect(growth).to.be.at.most(
        result.after1 * 0.1,
        `C++ heap grew by ${growth} bytes between two identical rounds of 100 menu rebuilds — likely a leak`
      );
    });
  });

  describe('internal event', () => {
    it('should record as node in heap snapshot', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string) => {
          const { BrowserWindow } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);

          const w = new BrowserWindow({
            show: false
          });
          await w.loadURL('about:blank');
          const state = recordState();
          const isClosed = once(w, 'closed');
          w.destroy();
          await isClosed;
          const eventNativeStackReference = containsRetainingPath(state.snapshot, [
            'C++ native stack roots',
            'Electron / Event'
          ]);
          const noPersistentReference = !containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / Event'
          ]);
          return eventNativeStackReference && noPersistentReference;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js')
      );
      expect(result).to.equal(true);
    });
  });

  describe('powerMonitor module', () => {
    it('should retain native PowerMonitor via JS module reference', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string) => {
          const { powerMonitor, app } = require('electron');
          const { recordState } = require(heap);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          // Register a listener to trigger native PowerMonitor creation.
          const listener = () => {};
          powerMonitor.on('suspend', listener);

          // Add and remove several listeners to exercise the path,
          // then GC to ensure no duplicate native objects are created.
          for (let i = 0; i < 5; i++) {
            const tmp = () => {};
            powerMonitor.on('resume', tmp);
            powerMonitor.removeListener('resume', tmp);
          }

          v8Util.requestGarbageCollectionForTesting();

          const state = recordState();
          const nodes = state.snapshot.filter((node: any) => node.name === 'Electron / PowerMonitor');
          const found = nodes.length > 0;
          const noDuplicates = nodes.length === 1;

          setTimeout(() => app.quit());
          return { found, noDuplicates };
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
      expect(result.found).to.equal(true, 'PowerMonitor should be in snapshot (held by JS module)');
      expect(result.noDuplicates).to.equal(true, 'should have exactly one PowerMonitor instance');
    });
  });

  describe('utilityProcess module', () => {
    it('should appear in heap snapshot while process is running', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string, fixturePath: string) => {
          const { utilityProcess } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);

          const child = utilityProcess.fork(fixturePath);
          await once(child, 'spawn');

          const state = recordState();
          const found = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / UtilityProcess']);

          child.kill();
          await once(child, 'exit');
          return found;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js'),
        path.join(__dirname, 'fixtures/api/utility-process/endless.js')
      );
      expect(result).to.equal(true);
    });

    it('should be released from heap snapshot after process exits', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string, snapshotHelper: string, fixturePath: string) => {
          const { utilityProcess } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          let child: any = utilityProcess.fork(fixturePath);
          await once(child, 'exit');
          child = null;

          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const found = containsRetainingPath(state.snapshot, ['C++ Persistent roots', 'Electron / UtilityProcess']);
          return !found;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js'),
        path.join(__dirname, 'fixtures/api/utility-process/empty.js')
      );
      expect(result).to.equal(true, 'UtilityProcess should be released after exit and GC');
    });

    it('should survive GC when JS reference is dropped but process is still running', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string, snapshotHelper: string, fixturePath: string) => {
          const { utilityProcess, app } = require('electron');
          const { once } = require('node:events');
          const { recordState } = require(heap);
          const { containsRetainingPath } = require(snapshotHelper);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          let child: any = utilityProcess.fork(fixturePath);
          await once(child, 'spawn');
          child = null;

          // Force GC — the process should still be alive because
          // SelfKeepAlive roots the C++ wrapper.
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }

          const state = recordState();
          const stillAlive = containsRetainingPath(state.snapshot, [
            'C++ Persistent roots',
            'Electron / UtilityProcess'
          ]);

          setTimeout(() => app.quit());
          return stillAlive;
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap'),
        path.join(__dirname, 'lib', 'heapsnapshot-helpers.js'),
        path.join(__dirname, 'fixtures/api/utility-process/endless.js')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
      expect(result).to.equal(true, 'UtilityProcess should survive GC while process is running');
    });

    it('should not leak when forking multiple processes', async () => {
      const { remotely } = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await remotely(
        async (fixturePath: string) => {
          const { utilityProcess } = require('electron');
          const { once } = require('node:events');
          const { getCppHeapStatistics } = require('node:v8');
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

          async function forkAndWait() {
            const child = utilityProcess.fork(fixturePath);
            await once(child, 'exit');
          }

          async function measure(n: number) {
            for (let i = 0; i < n; i++) {
              await forkAndWait();
            }
            for (let i = 0; i < 10; i++) {
              await new Promise((resolve) => setTimeout(resolve, 0));
              v8Util.requestGarbageCollectionForTesting();
            }
            return getCppHeapStatistics('brief').used_size_bytes;
          }

          await measure(5);
          const after1 = await measure(10);
          const after2 = await measure(10);
          return { after1, after2 };
        },
        path.join(__dirname, 'fixtures/api/utility-process/empty.js')
      );

      const growth = result.after2 - result.after1;
      expect(growth).to.be.at.most(
        result.after1 * 0.1,
        `C++ heap grew by ${growth} bytes between rounds — likely a leak`
      );
    });
  });

  ifdescribe(process.platform === 'darwin')('autoUpdater module', () => {
    it('is retained after garbage collection', async () => {
      const rc = await startRemoteControlApp(['--js-flags=--expose-gc']);
      const result = await rc.remotely(async () => {
        const { autoUpdater } = require('electron');
        const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
        const wr = new WeakRef(autoUpdater);
        for (let i = 0; i < 10; i++) {
          await new Promise((resolve) => setTimeout(resolve, 0));
          v8Util.requestGarbageCollectionForTesting();
        }
        return {
          retained: wr.deref() !== undefined,
          functional: typeof autoUpdater.getFeedURL() === 'string'
        };
      });
      expect(result.retained).to.equal(true, 'autoUpdater should survive GC');
      expect(result.functional).to.equal(true, 'autoUpdater should still be functional after GC');
    });

    it('should record as node in heap snapshot', async () => {
      const rc = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await rc.remotely(
        async (heap: string) => {
          const { autoUpdater, app } = require('electron');
          const { recordState } = require(heap);
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
          console.log(autoUpdater.getFeedURL());
          for (let i = 0; i < 10; i++) {
            await new Promise((resolve) => setTimeout(resolve, 0));
            v8Util.requestGarbageCollectionForTesting();
          }
          const state = recordState();
          const nodes = state.snapshot.filter((node: any) => node.name === 'Electron / AutoUpdater');
          const found = nodes.length > 0;
          const noDuplicates = nodes.length === 1;
          setTimeout(() => app.quit());
          return { found, noDuplicates };
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap')
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
      expect(result.found).to.equal(true, 'AutoUpdater should be in snapshot (held by JS module)');
      expect(result.noDuplicates).to.equal(true, 'should have exactly one AutoUpdater instance');
    });
  });

  ifdescribe(isTestingBindingAvailable())('gin_helper::Promise cppgc', () => {
    it('does not crash on exit with a live PromiseBase', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(async () => {
        const { app } = require('electron');
        const testingBinding = (process as any)._linkedBinding('electron_common_testing');

        // Create a gin_helper::Promise<void> and hold it in a static C++
        // variable. It will outlive the isolate and be destroyed during
        // static destruction — this must not crash.
        testingBinding.holdPromiseForTesting();

        setTimeout(() => app.quit());
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
    });

    it('PromiseHandle should not leak', async () => {
      const { remotely } = await startRemoteControlApp(['--expose-internals', '--js-flags=--expose-gc']);
      const result = await remotely(
        async (heap: string) => {
          const testingBinding = (process as any)._linkedBinding('electron_common_testing');
          const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
          const { recordState } = require(heap);

          const countPromiseHandles = () =>
            recordState().snapshot.filter((node: any) => node.name === 'Electron / PromiseHandle').length;

          const gc = async () => {
            for (let i = 0; i < 10; i++) {
              await new Promise((resolve) => setTimeout(resolve, 0));
              v8Util.requestGarbageCollectionForTesting();
            }
          };

          testingBinding.holdPromiseForTesting();
          const beforeGC = countPromiseHandles();
          await gc();
          const afterGC = countPromiseHandles();

          testingBinding.clearHeldPromiseForTesting();
          await gc();
          const afterClear = countPromiseHandles();

          return { beforeGC, afterGC, afterClear };
        },
        path.join(__dirname, '../../third_party/electron_node/test/common/heap')
      );

      expect(result.afterGC).to.be.at.least(result.beforeGC, 'held PromiseHandle must survive GC');
      expect(result.afterClear).to.be.lessThan(result.afterGC, 'clearing should release the PromiseHandle');
    });
  });

  describe('webFrame module', () => {
    itremote('does not leak WebFrameRenderer wrappers', async () => {
      const { webFrame } = require('electron');
      const { expect } = require('chai');
      const v8Util = (process as any)._linkedBinding('electron_common_v8_util');

      const refs: WeakRef<object>[] = (() => {
        const out: WeakRef<object>[] = [];
        for (let i = 0; i < 50; i++) {
          out.push(new WeakRef(webFrame.top as unknown as object));
        }
        return out;
      })();

      for (let i = 0; i < 10; i++) {
        await new Promise((resolve) => setTimeout(resolve, 0));
        v8Util.requestGarbageCollectionForTesting();
      }

      const survivors = refs.filter((ref) => ref.deref() !== undefined).length;
      expect(survivors).to.equal(0);
    });
  });
});
