import { ipcMain, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

import { closeWindow } from './lib/window-helpers';

describe('ipcRenderer module', () => {
  let w: BrowserWindow;
  before(async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        nodeIntegrationInSubFrames: true,
        contextIsolation: false
      }
    });
    await w.loadURL('about:blank');
    w.webContents.on('console-message', (event, ...args) => console.error(...args));
  });
  after(async () => {
    await closeWindow(w);
    w = null as unknown as BrowserWindow;
  });

  describe('send()', () => {
    it('should work when sending an object containing id property', async () => {
      const obj = {
        id: 1,
        name: 'ly'
      };
      w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')
        ipcRenderer.send('message', ${JSON.stringify(obj)})
      }`);
      const [, received] = await once(ipcMain, 'message');
      expect(received).to.deep.equal(obj);
    });

    it('can send instances of Date as Dates', async () => {
      const isoDate = new Date().toISOString();
      w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')
        ipcRenderer.send('message', new Date(${JSON.stringify(isoDate)}))
      }`);
      const [, received] = await once(ipcMain, 'message');
      expect(received.toISOString()).to.equal(isoDate);
    });

    it('can send instances of Buffer', async () => {
      const data = 'hello';
      w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')
        ipcRenderer.send('message', Buffer.from(${JSON.stringify(data)}))
      }`);
      const [, received] = await once(ipcMain, 'message');
      expect(received).to.be.an.instanceOf(Uint8Array);
      expect(Buffer.from(data).equals(received)).to.be.true();
    });

    it('throws when sending objects with DOM class prototypes', async () => {
      await expect(w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')
        ipcRenderer.send('message', document.location)
      }`)).to.eventually.be.rejected();
    });

    it('does not crash when sending external objects', async () => {
      await expect(w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')
        const http = require('node:http')

        const request = http.request({ port: 5000, hostname: '127.0.0.1', method: 'GET', path: '/' })
        const stream = request.agent.sockets['127.0.0.1:5000:'][0]._handle._externalStream

        ipcRenderer.send('message', stream)
      }`)).to.eventually.be.rejected();
    });

    it('can send objects that both reference the same object', async () => {
      w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')

        const child = { hello: 'world' }
        const foo = { name: 'foo', child: child }
        const bar = { name: 'bar', child: child }
        const array = [foo, bar]

        ipcRenderer.send('message', array, foo, bar, child)
      }`);

      const child = { hello: 'world' };
      const foo = { name: 'foo', child };
      const bar = { name: 'bar', child };
      const array = [foo, bar];

      const [, arrayValue, fooValue, barValue, childValue] = await once(ipcMain, 'message');
      expect(arrayValue).to.deep.equal(array);
      expect(fooValue).to.deep.equal(foo);
      expect(barValue).to.deep.equal(bar);
      expect(childValue).to.deep.equal(child);
    });

    it('can handle cyclic references', async () => {
      w.webContents.executeJavaScript(`{
        const { ipcRenderer } = require('electron')
        const array = [5]
        array.push(array)

        const child = { hello: 'world' }
        child.child = child
        ipcRenderer.send('message', array, child)
      }`);

      const [, arrayValue, childValue] = await once(ipcMain, 'message');
      expect(arrayValue[0]).to.equal(5);
      expect(arrayValue[1]).to.equal(arrayValue);

      expect(childValue.hello).to.equal('world');
      expect(childValue.child).to.equal(childValue);
    });
  });

  describe('sendSync()', () => {
    it('can be replied to by setting event.returnValue', async () => {
      ipcMain.once('echo', (event, msg) => {
        event.returnValue = msg;
      });
      const msg = await w.webContents.executeJavaScript(`new Promise(resolve => {
        const { ipcRenderer } = require('electron')
        resolve(ipcRenderer.sendSync('echo', 'test'))
      })`);
      expect(msg).to.equal('test');
    });
  });

  describe('ipcRenderer.on', () => {
    it('is not used for internals', async () => {
      const result = await w.webContents.executeJavaScript(`
        require('electron').ipcRenderer.eventNames()
      `);
      expect(result).to.deep.equal([]);
    });
  });

  describe('ipcRenderer.removeAllListeners', () => {
    it('removes only the given channel', async () => {
      const result = await w.webContents.executeJavaScript(`
        (() => {
          const { ipcRenderer } = require('electron');

          ipcRenderer.on('channel1', () => {});
          ipcRenderer.on('channel2', () => {});

          ipcRenderer.removeAllListeners('channel1');

          return ipcRenderer.eventNames();
        })()
      `);
      expect(result).to.deep.equal(['channel2']);
    });

    it('removes all channels if no channel is specified', async () => {
      const result = await w.webContents.executeJavaScript(`
        (() => {
          const { ipcRenderer } = require('electron');

          ipcRenderer.on('channel1', () => {});
          ipcRenderer.on('channel2', () => {});

          ipcRenderer.removeAllListeners();

          return ipcRenderer.eventNames();
        })()
      `);
      expect(result).to.deep.equal([]);
    });
  });

  describe('after context is released', () => {
    it('throws an exception', async () => {
      const error = await w.webContents.executeJavaScript(`(${() => {
        const child = window.open('', 'child', 'show=no,nodeIntegration=yes')! as any;
        const childIpc = child.require('electron').ipcRenderer;
        child.close();
        return new Promise(resolve => {
          setInterval(() => {
            try {
              childIpc.send('hello');
            } catch (e) {
              resolve(e);
            }
          }, 16);
        });
      }})()`);
      expect(error).to.have.property('message', 'IPC method called after context was released');
    });
  });
});
