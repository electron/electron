import { BrowserWindow, ipcMain, IpcMainInvokeEvent, MessageChannelMain, WebContents, ipcStreamMain } from 'electron/main';

import { expect } from 'chai';

import { EventEmitter, once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';

import { defer, listen } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('ipc-stream module', () => {
  afterEach(closeAllWindows);

  describe('basic functionality', () => {
    it('should create a server and client', () => {
      const server = ipcStreamMain.createServer();
      const client = ipcStreamMain.createClient();
      
      expect(server).to.be.an.instanceOf(EventEmitter);
      expect(client).to.be.an('object');
    });

    it('should listen on a channel', () => {
      const server = ipcStreamMain.createServer();
      
      expect(server.isListening('test-channel')).to.be.false();
      
      server.listen('test-channel', () => {});
      expect(server.isListening('test-channel')).to.be.true();
      
      server.unlisten('test-channel');
      expect(server.isListening('test-channel')).to.be.false();
    });

    it('should throw when listening on the same channel twice', () => {
      const server = ipcStreamMain.createServer();
      
      server.listen('test-channel', () => {});
      
      expect(() => {
        server.listen('test-channel', () => {});
      }).to.throw(/already in use/);
    });
  });

  describe('main process to renderer communication', () => {
    it('should establish a stream connection between main and renderer', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const connectionPromise = new Promise<void>((resolve) => {
        server.listen('test-stream', (stream) => {
          expect(stream).to.be.an.instanceOf(EventEmitter);
          expect(stream.id).to.be.a('string');
          expect(stream.isOpen).to.be.true();
          resolve();
        });
      });

      await w.webContents.executeJavaScript(`
        (${function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          client.connect('test-stream');
        }})()
      `);

      await connectionPromise;
    });

    it('should send and receive data through the stream', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const dataPromise = new Promise<any>((resolve) => {
        server.listen('data-stream', (stream) => {
          stream.on('data', (data) => {
            resolve(data);
          });
        });
      });

      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          const stream = await client.connect('data-stream');
          stream.write({ test: 'data', number: 42 });
        }})()
      `);

      const receivedData = await dataPromise;
      expect(receivedData).to.deep.equal({ test: 'data', number: 42 });
    });

    it('should handle stream end event', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const endPromise = new Promise<void>((resolve) => {
        server.listen('end-stream', (stream) => {
          stream.on('end', () => {
            resolve();
          });
        });
      });

      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          const stream = await client.connect('end-stream');
          stream.end();
        }})()
      `);

      await endPromise;
    });

    it('should handle stream error event', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const errorPromise = new Promise<Error>((resolve) => {
        server.listen('error-stream', (stream) => {
          stream.on('error', (error) => {
            resolve(error);
          });
        });
      });

      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          const stream = await client.connect('error-stream');
          stream.error(new Error('Test error message'));
        }})()
      `);

      const error = await errorPromise;
      expect(error).to.be.an.instanceOf(Error);
      expect(error.message).to.equal('Test error message');
    });

    it('should support ping/pong mechanism', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const pongPromise = new Promise<void>((resolve) => {
        server.listen('ping-stream', (stream) => {
          stream.on('pong', () => {
            resolve();
          });
          stream.ping();
        });
      });

      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          await client.connect('ping-stream');
        }})()
      `);

      await pongPromise;
    });

    it('should send data from main to renderer', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      
      // Set up listener in renderer first
      await w.webContents.executeJavaScript(`
        (${function () {
          const { ipcStreamRenderer } = require('electron');
          const server = ipcStreamRenderer.createServer();
          server.listen('main-to-renderer', (stream) => {
            stream.on('data', (data) => {
              require('electron').ipcRenderer.send('received-data', data);
            });
          });
        }})()
      `);

      const dataPromise = once(ipcMain, 'received-data');
      
      // Connect from main process
      const client = ipcStreamMain.createClient();
      const stream = await client.connect(w.webContents, 'main-to-renderer');
      stream.write({ from: 'main', message: 'Hello from main process!' });

      const [, receivedData] = await dataPromise;
      expect(receivedData).to.deep.equal({ from: 'main', message: 'Hello from main process!' });
    });
  });

  describe('stream options', () => {
    it('should support object mode', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const objectPromise = new Promise<any>((resolve) => {
        server.listen('object-stream', (stream) => {
          stream.on('data', (data) => {
            resolve(data);
          });
        });
      });

      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          const stream = await client.connect('object-stream', { objectMode: true });
          
          // Send complex objects
          const complexObject = {
            nested: {
              array: [1, 2, 3],
              obj: { key: 'value' }
            },
            date: new Date('2023-01-01'),
            regex: /test/gi
          };
          
          stream.write(complexObject);
        }})()
      `);

      const received = await objectPromise;
      expect(received.nested.array).to.deep.equal([1, 2, 3]);
      expect(received.nested.obj.key).to.equal('value');
    });

    it('should reject connection to non-existent channel', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const errorPromise = new Promise<string>((resolve) => {
        ipcMain.once('connection-error', (event, errorMessage) => {
          resolve(errorMessage);
        });
      });

      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          try {
            await client.connect('non-existent-channel');
          } catch (error) {
            require('electron').ipcRenderer.send('connection-error', error.message);
          }
        }})()
      `);

      const errorMessage = await errorPromise;
      expect(errorMessage).to.include('non-existent-channel');
    });
  });

  describe('bidirectional communication', () => {
    it('should support bidirectional data flow', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const responsePromise = new Promise<number>((resolve) => {
        server.listen('echo-stream', (stream) => {
          stream.on('data', (data) => {
            // Echo back the data multiplied by 2
            stream.write(data * 2);
          });
        });
      });

      // Set up renderer to send data and listen for response
      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          const stream = await client.connect('echo-stream');
          
          stream.on('data', (data) => {
            require('electron').ipcRenderer.send('echo-response', data);
          });
          
          stream.write(21);
        }})()
      `);

      const [, response] = await once(ipcMain, 'echo-response');
      expect(response).to.equal(42);
    });

    it('should handle multiple concurrent streams', async () => {
      const w = new BrowserWindow({ 
        show: false, 
        webPreferences: { 
          nodeIntegration: true, 
          contextIsolation: false 
        } 
      });
      await w.loadURL('about:blank');

      const server = ipcStreamMain.createServer();
      const streams: any[] = [];
      
      server.listen('multi-stream', (stream) => {
        streams.push(stream);
        stream.on('data', (data) => {
          stream.write(`Received: ${data}`);
        });
      });

      // Set up renderer to create multiple connections
      await w.webContents.executeJavaScript(`
        (${async function () {
          const { ipcStreamRenderer } = require('electron');
          const client = ipcStreamRenderer.createClient();
          
          // Create 3 concurrent streams
          for (let i = 0; i < 3; i++) {
            const stream = await client.connect('multi-stream');
            stream.on('data', (data) => {
              require('electron').ipcRenderer.send('multi-response', data);
            });
            stream.write(\`Message \${i}\`);
          }
        }})()
      `);

      // Wait for all 3 responses
      const responses: string[] = [];
      for (let i = 0; i < 3; i++) {
        const [, data] = await once(ipcMain, 'multi-response');
        responses.push(data);
      }

      expect(responses).to.have.length(3);
      expect(responses[0]).to.include('Received');
      expect(responses[1]).to.include('Received');
      expect(responses[2]).to.include('Received');
    });
  });
});
