import { EventEmitter } from 'events';
import { expect } from 'chai';
import { BrowserWindow, ipcMain, IpcMainInvokeEvent, MessageChannelMain, WebContents } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';

const v8Util = process._linkedBinding('electron_common_v8_util');

describe('ipc module', () => {
  describe('invoke', () => {
    let w = (null as unknown as BrowserWindow);

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadURL('about:blank');
    });
    after(async () => {
      w.destroy();
    });

    async function rendererInvoke (...args: any[]) {
      const { ipcRenderer } = require('electron');
      try {
        const result = await ipcRenderer.invoke('test', ...args);
        ipcRenderer.send('result', { result });
      } catch (e) {
        ipcRenderer.send('result', { error: e.message });
      }
    }

    it('receives a response from a synchronous handler', async () => {
      ipcMain.handleOnce('test', (e: IpcMainInvokeEvent, arg: number) => {
        expect(arg).to.equal(123);
        return 3;
      });
      const done = new Promise<void>(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({ result: 3 });
        resolve();
      }));
      await w.webContents.executeJavaScript(`(${rendererInvoke})(123)`);
      await done;
    });

    it('receives a response from an asynchronous handler', async () => {
      ipcMain.handleOnce('test', async (e: IpcMainInvokeEvent, arg: number) => {
        expect(arg).to.equal(123);
        await new Promise(setImmediate);
        return 3;
      });
      const done = new Promise<void>(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({ result: 3 });
        resolve();
      }));
      await w.webContents.executeJavaScript(`(${rendererInvoke})(123)`);
      await done;
    });

    it('receives an error from a synchronous handler', async () => {
      ipcMain.handleOnce('test', () => {
        throw new Error('some error');
      });
      const done = new Promise<void>(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/some error/);
        resolve();
      }));
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`);
      await done;
    });

    it('receives an error from an asynchronous handler', async () => {
      ipcMain.handleOnce('test', async () => {
        await new Promise(setImmediate);
        throw new Error('some error');
      });
      const done = new Promise<void>(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/some error/);
        resolve();
      }));
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`);
      await done;
    });

    it('throws an error if no handler is registered', async () => {
      const done = new Promise<void>(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/No handler registered/);
        resolve();
      }));
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`);
      await done;
    });

    it('throws an error when invoking a handler that was removed', async () => {
      ipcMain.handle('test', () => {});
      ipcMain.removeHandler('test');
      const done = new Promise<void>(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/No handler registered/);
        resolve();
      }));
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`);
      await done;
    });

    it('forbids multiple handlers', async () => {
      ipcMain.handle('test', () => {});
      try {
        expect(() => { ipcMain.handle('test', () => {}); }).to.throw(/second handler/);
      } finally {
        ipcMain.removeHandler('test');
      }
    });

    it('throws an error in the renderer if the reply callback is dropped', async () => {
      // eslint-disable-next-line @typescript-eslint/no-unused-vars
      ipcMain.handleOnce('test', () => new Promise(resolve => {
        setTimeout(() => v8Util.requestGarbageCollectionForTesting());
        /* never resolve */
      }));
      w.webContents.executeJavaScript(`(${rendererInvoke})()`);
      const [, { error }] = await emittedOnce(ipcMain, 'result');
      expect(error).to.match(/reply was never sent/);
    });
  });

  describe('ordering', () => {
    let w = (null as unknown as BrowserWindow);

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadURL('about:blank');
    });
    after(async () => {
      w.destroy();
    });

    it('between send and sendSync is consistent', async () => {
      const received: number[] = [];
      ipcMain.on('test-async', (e, i) => { received.push(i); });
      ipcMain.on('test-sync', (e, i) => { received.push(i); e.returnValue = null; });
      const done = new Promise<void>(resolve => ipcMain.once('done', () => { resolve(); }));
      function rendererStressTest () {
        const { ipcRenderer } = require('electron');
        for (let i = 0; i < 1000; i++) {
          switch ((Math.random() * 2) | 0) {
            case 0:
              ipcRenderer.send('test-async', i);
              break;
            case 1:
              ipcRenderer.sendSync('test-sync', i);
              break;
          }
        }
        ipcRenderer.send('done');
      }
      try {
        w.webContents.executeJavaScript(`(${rendererStressTest})()`);
        await done;
      } finally {
        ipcMain.removeAllListeners('test-async');
        ipcMain.removeAllListeners('test-sync');
      }
      expect(received).to.have.lengthOf(1000);
      expect(received).to.deep.equal([...received].sort((a, b) => a - b));
    });

    it('between send, sendSync, and invoke is consistent', async () => {
      const received: number[] = [];
      ipcMain.handle('test-invoke', (e, i) => { received.push(i); });
      ipcMain.on('test-async', (e, i) => { received.push(i); });
      ipcMain.on('test-sync', (e, i) => { received.push(i); e.returnValue = null; });
      const done = new Promise<void>(resolve => ipcMain.once('done', () => { resolve(); }));
      function rendererStressTest () {
        const { ipcRenderer } = require('electron');
        for (let i = 0; i < 1000; i++) {
          switch ((Math.random() * 3) | 0) {
            case 0:
              ipcRenderer.send('test-async', i);
              break;
            case 1:
              ipcRenderer.sendSync('test-sync', i);
              break;
            case 2:
              ipcRenderer.invoke('test-invoke', i);
              break;
          }
        }
        ipcRenderer.send('done');
      }
      try {
        w.webContents.executeJavaScript(`(${rendererStressTest})()`);
        await done;
      } finally {
        ipcMain.removeHandler('test-invoke');
        ipcMain.removeAllListeners('test-async');
        ipcMain.removeAllListeners('test-sync');
      }
      expect(received).to.have.lengthOf(1000);
      expect(received).to.deep.equal([...received].sort((a, b) => a - b));
    });
  });

  describe('MessagePort', () => {
    afterEach(closeAllWindows);

    it('can send a port to the main process', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      const p = emittedOnce(ipcMain, 'port');
      await w.webContents.executeJavaScript(`(${function () {
        const channel = new MessageChannel();
        require('electron').ipcRenderer.postMessage('port', 'hi', [channel.port1]);
      }})()`);
      const [ev, msg] = await p;
      expect(msg).to.equal('hi');
      expect(ev.ports).to.have.length(1);
      const [port] = ev.ports;
      expect(port).to.be.an.instanceOf(EventEmitter);
    });

    it('can communicate between main and renderer', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      const p = emittedOnce(ipcMain, 'port');
      await w.webContents.executeJavaScript(`(${function () {
        const channel = new MessageChannel();
        (channel.port2 as any).onmessage = (ev: any) => {
          channel.port2.postMessage(ev.data * 2);
        };
        require('electron').ipcRenderer.postMessage('port', '', [channel.port1]);
      }})()`);
      const [ev] = await p;
      expect(ev.ports).to.have.length(1);
      const [port] = ev.ports;
      port.start();
      port.postMessage(42);
      const [ev2] = await emittedOnce(port, 'message');
      expect(ev2.data).to.equal(84);
    });

    it('can receive a port from a renderer over a MessagePort connection', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      function fn () {
        const channel1 = new MessageChannel();
        const channel2 = new MessageChannel();
        channel1.port2.postMessage('', [channel2.port1]);
        channel2.port2.postMessage('matryoshka');
        require('electron').ipcRenderer.postMessage('port', '', [channel1.port1]);
      }
      w.webContents.executeJavaScript(`(${fn})()`);
      const [{ ports: [port1] }] = await emittedOnce(ipcMain, 'port');
      port1.start();
      const [{ ports: [port2] }] = await emittedOnce(port1, 'message');
      port2.start();
      const [{ data }] = await emittedOnce(port2, 'message');
      expect(data).to.equal('matryoshka');
    });

    it('can forward a port from one renderer to another renderer', async () => {
      const w1 = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      const w2 = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w1.loadURL('about:blank');
      w2.loadURL('about:blank');
      w1.webContents.executeJavaScript(`(${function () {
        const channel = new MessageChannel();
        (channel.port2 as any).onmessage = (ev: any) => {
          require('electron').ipcRenderer.send('message received', ev.data);
        };
        require('electron').ipcRenderer.postMessage('port', '', [channel.port1]);
      }})()`);
      const [{ ports: [port] }] = await emittedOnce(ipcMain, 'port');
      await w2.webContents.executeJavaScript(`(${function () {
        require('electron').ipcRenderer.on('port', ({ ports: [port] }: any) => {
          port.postMessage('a message');
        });
      }})()`);
      w2.webContents.postMessage('port', '', [port]);
      const [, data] = await emittedOnce(ipcMain, 'message received');
      expect(data).to.equal('a message');
    });

    describe('close event', () => {
      describe('in renderer', () => {
        it('is emitted when the main process closes its end of the port', async () => {
          const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
          w.loadURL('about:blank');
          await w.webContents.executeJavaScript(`(${function () {
            const { ipcRenderer } = require('electron');
            ipcRenderer.on('port', e => {
              const [port] = e.ports;
              port.start();
              (port as any).onclose = () => {
                ipcRenderer.send('closed');
              };
            });
          }})()`);
          const { port1, port2 } = new MessageChannelMain();
          w.webContents.postMessage('port', null, [port2]);
          port1.close();
          await emittedOnce(ipcMain, 'closed');
        });

        it('is emitted when the other end of a port is garbage-collected', async () => {
          const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
          w.loadURL('about:blank');
          await w.webContents.executeJavaScript(`(${async function () {
            const { port2 } = new MessageChannel();
            await new Promise(resolve => {
              port2.start();
              (port2 as any).onclose = resolve;
              process._linkedBinding('electron_common_v8_util').requestGarbageCollectionForTesting();
            });
          }})()`);
        });

        it('is emitted when the other end of a port is sent to nowhere', async () => {
          const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
          w.loadURL('about:blank');
          ipcMain.once('do-a-gc', () => v8Util.requestGarbageCollectionForTesting());
          await w.webContents.executeJavaScript(`(${async function () {
            const { port1, port2 } = new MessageChannel();
            await new Promise(resolve => {
              port2.start();
              (port2 as any).onclose = resolve;
              require('electron').ipcRenderer.postMessage('nobody-listening', null, [port1]);
              require('electron').ipcRenderer.send('do-a-gc');
            });
          }})()`);
        });
      });
    });

    describe('MessageChannelMain', () => {
      it('can be created', () => {
        const { port1, port2 } = new MessageChannelMain();
        expect(port1).not.to.be.null();
        expect(port2).not.to.be.null();
      });

      it('can send messages within the process', async () => {
        const { port1, port2 } = new MessageChannelMain();
        port2.postMessage('hello');
        port1.start();
        const [ev] = await emittedOnce(port1, 'message');
        expect(ev.data).to.equal('hello');
      });

      it('can pass one end to a WebContents', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript(`(${function () {
          const { ipcRenderer } = require('electron');
          ipcRenderer.on('port', ev => {
            const [port] = ev.ports;
            port.onmessage = () => {
              ipcRenderer.send('done');
            };
          });
        }})()`);
        const { port1, port2 } = new MessageChannelMain();
        port1.postMessage('hello');
        w.webContents.postMessage('port', null, [port2]);
        await emittedOnce(ipcMain, 'done');
      });

      it('can be passed over another channel', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript(`(${function () {
          const { ipcRenderer } = require('electron');
          ipcRenderer.on('port', e1 => {
            e1.ports[0].onmessage = e2 => {
              e2.ports[0].onmessage = e3 => {
                ipcRenderer.send('done', e3.data);
              };
            };
          });
        }})()`);
        const { port1, port2 } = new MessageChannelMain();
        const { port1: port3, port2: port4 } = new MessageChannelMain();
        port1.postMessage(null, [port4]);
        port3.postMessage('hello');
        w.webContents.postMessage('port', null, [port2]);
        const [, message] = await emittedOnce(ipcMain, 'done');
        expect(message).to.equal('hello');
      });

      it('can send messages to a closed port', () => {
        const { port1, port2 } = new MessageChannelMain();
        port2.start();
        port2.on('message', () => { throw new Error('unexpected message received'); });
        port1.close();
        port1.postMessage('hello');
      });

      it('can send messages to a port whose remote end is closed', () => {
        const { port1, port2 } = new MessageChannelMain();
        port2.start();
        port2.on('message', () => { throw new Error('unexpected message received'); });
        port2.close();
        port1.postMessage('hello');
      });

      it('throws when passing null ports', () => {
        const { port1 } = new MessageChannelMain();
        expect(() => {
          port1.postMessage(null, [null] as any);
        }).to.throw(/conversion failure/);
      });

      it('throws when passing duplicate ports', () => {
        const { port1 } = new MessageChannelMain();
        const { port1: port3 } = new MessageChannelMain();
        expect(() => {
          port1.postMessage(null, [port3, port3]);
        }).to.throw(/duplicate/);
      });

      it('throws when passing ports that have already been neutered', () => {
        const { port1 } = new MessageChannelMain();
        const { port1: port3 } = new MessageChannelMain();
        port1.postMessage(null, [port3]);
        expect(() => {
          port1.postMessage(null, [port3]);
        }).to.throw(/already neutered/);
      });

      it('throws when passing itself', () => {
        const { port1 } = new MessageChannelMain();
        expect(() => {
          port1.postMessage(null, [port1]);
        }).to.throw(/contains the source port/);
      });

      describe('GC behavior', () => {
        it('is not collected while it could still receive messages', async () => {
          let trigger: Function;
          const promise = new Promise(resolve => { trigger = resolve; });
          const port1 = (() => {
            const { port1, port2 } = new MessageChannelMain();

            port2.on('message', (e) => { trigger(e.data); });
            port2.start();
            return port1;
          })();
          v8Util.requestGarbageCollectionForTesting();
          port1.postMessage('hello');
          expect(await promise).to.equal('hello');
        });
      });
    });

    const generateTests = (title: string, postMessage: (contents: WebContents) => WebContents['postMessage']) => {
      describe(title, () => {
        it('sends a message', async () => {
          const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
          w.loadURL('about:blank');
          await w.webContents.executeJavaScript(`(${function () {
            const { ipcRenderer } = require('electron');
            ipcRenderer.on('foo', (_e, msg) => {
              ipcRenderer.send('bar', msg);
            });
          }})()`);
          postMessage(w.webContents)('foo', { some: 'message' });
          const [, msg] = await emittedOnce(ipcMain, 'bar');
          expect(msg).to.deep.equal({ some: 'message' });
        });

        describe('error handling', () => {
          it('throws on missing channel', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              (postMessage(w.webContents) as any)();
            }).to.throw(/Insufficient number of arguments/);
          });

          it('throws on invalid channel', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              postMessage(w.webContents)(null as any, '', []);
            }).to.throw(/Error processing argument at index 0/);
          });

          it('throws on missing message', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              (postMessage(w.webContents) as any)('channel');
            }).to.throw(/Insufficient number of arguments/);
          });

          it('throws on non-serializable message', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              postMessage(w.webContents)('channel', w);
            }).to.throw(/An object could not be cloned/);
          });

          it('throws on invalid transferable list', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              postMessage(w.webContents)('', '', null as any);
            }).to.throw(/Invalid value for transfer/);
          });

          it('throws on transferring non-transferable', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              (postMessage(w.webContents) as any)('channel', '', [123]);
            }).to.throw(/Invalid value for transfer/);
          });

          it('throws when passing null ports', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            expect(() => {
              postMessage(w.webContents)('foo', null, [null] as any);
            }).to.throw(/Invalid value for transfer/);
          });

          it('throws when passing duplicate ports', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            const { port1 } = new MessageChannelMain();
            expect(() => {
              postMessage(w.webContents)('foo', null, [port1, port1]);
            }).to.throw(/duplicate/);
          });

          it('throws when passing ports that have already been neutered', async () => {
            const w = new BrowserWindow({ show: false });
            await w.loadURL('about:blank');
            const { port1 } = new MessageChannelMain();
            postMessage(w.webContents)('foo', null, [port1]);
            expect(() => {
              postMessage(w.webContents)('foo', null, [port1]);
            }).to.throw(/already neutered/);
          });
        });
      });
    };

    generateTests('WebContents.postMessage', contents => contents.postMessage.bind(contents));
    generateTests('WebFrameMain.postMessage', contents => contents.mainFrame.postMessage.bind(contents.mainFrame));
  });
});
