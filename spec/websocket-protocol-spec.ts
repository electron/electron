import { protocol, BrowserWindow } from 'electron/main';
import { expect } from 'chai';
import * as WebSocket from 'ws';
import * as http from 'node:http';
import { closeWindow } from './lib/window-helpers';

describe('custom protocol WebSocket support', () => {
    const scheme = 'ws-custom';
    let server: http.Server;
    let wss: WebSocket.Server;
    let port: number;

    before(async () => {
        server = http.createServer();
        wss = new WebSocket.Server({ noServer: true });
        server.on('upgrade', (request, socket, head) => {
            wss.handleUpgrade(request, socket, head, (ws) => {
                ws.send('hello');
            });
        });
        await new Promise<void>(resolve => server.listen(0, '127.0.0.1', () => resolve()));
        port = (server.address() as any).port;
    });

    after(() => {
        wss.close();
        server.close();
    });

    afterEach(async () => {
        protocol.unhandle(scheme);
        if (w) await closeWindow(w);
    });

    let w: BrowserWindow;

    it('allows a custom scheme to redirect a WebSocket connection', async () => {
        protocol.handle(scheme, () => {
            return new Response(null, {
                status: 302,
                headers: {
                    Location: `ws://127.0.0.1:${port}`
                }
            });
        });

        w = new BrowserWindow({
            show: false,
            webPreferences: {
                nodeIntegration: true,
                contextIsolation: false
            }
        });

        const result = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        try {
          const ws = new WebSocket('${scheme}://example');
          ws.onmessage = (event) => {
            resolve(event.data);
            ws.close();
          };
          ws.onclose = () => {
            reject(new Error('WebSocket closed early'));
          };
          ws.onerror = (err) => {
            reject(new Error('WebSocket error'));
          };
          setTimeout(() => reject(new Error('Timeout')), 5000);
        } catch (e) {
          reject(e);
        }
      });
    `);

        expect(result).to.equal('hello');
    });

    it('allows a custom scheme to return a string URL for redirection', async () => {
        (protocol as any).registerStringProtocol(scheme, (request: any, callback: any) => {
            callback({ url: `ws://127.0.0.1:${port}` });
        });

        w = new BrowserWindow({
            show: false,
            webPreferences: {
                nodeIntegration: true,
                contextIsolation: false
            }
        });

        const result = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const ws = new WebSocket('${scheme}://example');
        ws.onmessage = (event) => {
          resolve(event.data);
          ws.close();
        };
        ws.onerror = () => reject(new Error('WebSocket error'));
        setTimeout(() => reject(new Error('Timeout')), 5000);
      });
    `);

        expect(result).to.equal('hello');
    });
});
