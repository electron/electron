import { BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import * as http from 'node:http';

import { listen } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

describe('webContents1c module', () => {
  describe('webContents.executeJavaScript', () => {
    describe('in about:blank', () => {
      const expected = 'hello, world!';
      const expectedErrorMsg = 'woops!';
      const code = `(() => "${expected}")()`;
      const asyncCode = `(() => new Promise(r => setTimeout(() => r("${expected}"), 500)))()`;
      const badAsyncCode = `(() => new Promise((r, e) => setTimeout(() => e("${expectedErrorMsg}"), 500)))()`;
      const errorTypes = new Set([
        Error,
        ReferenceError,
        EvalError,
        RangeError,
        SyntaxError,
        TypeError,
        URIError
      ]);
      let w: BrowserWindow;

      before(async () => {
        w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false } });
        await w.loadURL('about:blank');
      });
      after(closeAllWindows);

      it('resolves the returned promise with the result', async () => {
        const result = await w.webContents.executeJavaScript(code);
        expect(result).to.equal(expected);
      });
      it('resolves the returned promise with the result if the code returns an asynchronous promise', async () => {
        const result = await w.webContents.executeJavaScript(asyncCode);
        expect(result).to.equal(expected);
      });
      it('rejects the returned promise if an async error is thrown', async () => {
        await expect(w.webContents.executeJavaScript(badAsyncCode)).to.eventually.be.rejectedWith(expectedErrorMsg);
      });
      it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
        for (const error of errorTypes) {
          await expect(w.webContents.executeJavaScript(`Promise.reject(new ${error.name}("Wamp-wamp"))`))
            .to.eventually.be.rejectedWith(error);
        }
      });
    });

    describe('on a real page', () => {
      let w: BrowserWindow;
      beforeEach(() => {
        w = new BrowserWindow({ show: false });
      });
      afterEach(closeAllWindows);

      let server: http.Server;
      let serverUrl: string;

      before(async () => {
        server = http.createServer((request, response) => {
          response.end();
        });
        serverUrl = (await listen(server)).url;
      });

      after(() => {
        server.close();
        server = null as unknown as http.Server;
      });

      it('works after page load and during subframe load', async () => {
        await w.loadURL(serverUrl);
        // initiate a sub-frame load, then try and execute script during it
        await w.webContents.executeJavaScript(`
          var iframe = document.createElement('iframe')
          iframe.src = '${serverUrl}/slow'
          document.body.appendChild(iframe)
          null // don't return the iframe
        `);
        await w.webContents.executeJavaScript('console.log(\'hello\')');
      });

      it('executes after page load', async () => {
        const executeJavaScript = w.webContents.executeJavaScript('(() => "test")()');
        w.loadURL(serverUrl);
        const result = await executeJavaScript;
        expect(result).to.equal('test');
      });
    });
  });

  describe('webContents.executeJavaScriptInIsolatedWorld', () => {
    let w: BrowserWindow;

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadURL('about:blank');
    });

    it('resolves the returned promise with the result', async () => {
      await w.webContents.executeJavaScriptInIsolatedWorld(999, [{ code: 'window.X = 123' }]);
      const isolatedResult = await w.webContents.executeJavaScriptInIsolatedWorld(999, [{ code: 'window.X' }]);
      const mainWorldResult = await w.webContents.executeJavaScript('window.X');
      expect(isolatedResult).to.equal(123);
      expect(mainWorldResult).to.equal(undefined);
    });
  });
});
