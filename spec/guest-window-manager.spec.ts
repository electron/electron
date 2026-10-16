import { BrowserWindow, screen, session } from 'electron';

import { afterAll, afterEach, assert, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as http from 'node:http';
import * as nodePath from 'node:path';

import { HexColors, ScreenCapture, hasCapturableScreen } from './lib/screen-helpers.ts';
import { ifit, listen, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('webContents.setWindowOpenHandler', () => {
  describe('native window', () => {
    let browserWindow: BrowserWindow;
    beforeEach(async () => {
      browserWindow = new BrowserWindow({ show: false });
      await browserWindow.loadURL('about:blank');
    });

    afterEach(closeAllWindows);

    it('does not fire window creation events if the handler callback throws an error', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const error = new Error('oh no');
        const listeners = process.listeners('uncaughtException');
        process.removeAllListeners('uncaughtException');
        process.on('uncaughtException', (thrown) => {
          try {
            expect(thrown).toBe(error);
            done();
          } catch (e) {
            done(e);
          } finally {
            process.removeAllListeners('uncaughtException');
            for (const listener of listeners) {
              process.on('uncaughtException', listener);
            }
          }
        });

        browserWindow.webContents.on('did-create-window', () => {
          assert.fail('did-create-window should not be called with an overridden window.open');
        });

        browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");

        browserWindow.webContents.setWindowOpenHandler(() => {
          throw error;
        });
      }));

    it('does not fire window creation events if the handler callback returns a bad result', async () => {
      const bad = new Promise((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          setTimeout(resolve);
          return [1, 2, 3] as any;
        });
      });

      browserWindow.webContents.on('did-create-window', () => {
        assert.fail('did-create-window should not be called with an overridden window.open');
      });

      browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");

      await bad;
    });

    it('does not fire window creation events if an override returns action: deny', async () => {
      const denied = new Promise((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          setTimeout(resolve);
          return { action: 'deny' };
        });
      });

      browserWindow.webContents.on('did-create-window', () => {
        assert.fail('did-create-window should not be called with an overridden window.open');
      });

      browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");

      await denied;
    });

    it('is called when clicking on a target=_blank link', async () => {
      const denied = new Promise((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          setTimeout(resolve);
          return { action: 'deny' };
        });
      });

      browserWindow.webContents.on('did-create-window', () => {
        assert.fail('did-create-window should not be called with an overridden window.open');
      });

      await browserWindow.webContents.loadURL(
        'data:text/html,<a target="_blank" href="http://example.com" style="display: block; width: 100%; height: 100%; position: fixed; left: 0; top: 0;">link</a>'
      );
      browserWindow.webContents.sendInputEvent({ type: 'mouseDown', x: 10, y: 10, button: 'left', clickCount: 1 });
      browserWindow.webContents.sendInputEvent({ type: 'mouseUp', x: 10, y: 10, button: 'left', clickCount: 1 });

      await denied;
    });

    it('is called when shift-clicking on a link', async () => {
      const denied = new Promise((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          setTimeout(resolve);
          return { action: 'deny' };
        });
      });

      browserWindow.webContents.on('did-create-window', () => {
        assert.fail('did-create-window should not be called with an overridden window.open');
      });

      await browserWindow.webContents.loadURL(
        'data:text/html,<a href="http://example.com" style="display: block; width: 100%; height: 100%; position: fixed; left: 0; top: 0;">link</a>'
      );
      browserWindow.webContents.sendInputEvent({
        type: 'mouseDown',
        x: 10,
        y: 10,
        button: 'left',
        clickCount: 1,
        modifiers: ['shift']
      });
      browserWindow.webContents.sendInputEvent({
        type: 'mouseUp',
        x: 10,
        y: 10,
        button: 'left',
        clickCount: 1,
        modifiers: ['shift']
      });

      await denied;
    });

    describe('window opened by middle-clicking a link', () => {
      // These windows are opened by the browser process (OpenURLFromTab) rather
      // than by the renderer through window.open(), but must inherit from the
      // opener the same way.
      let server: http.Server;
      let url: string;
      beforeAll(async () => {
        server = http.createServer((req, res) => {
          res.setHeader('Content-Type', 'text/html');
          if (req.url === '/popup') {
            res.end('<title>no-script</title><script>document.title = "script-ran";</script>');
          } else {
            res.end(
              '<a href="/popup" style="display: block; width: 100%; height: 100%; position: fixed; left: 0; top: 0;">link</a>'
            );
          }
        });
        url = (await listen(server)).url;
      });
      afterAll(() => {
        server.close();
      });

      const middleClickLink = async (opener: BrowserWindow) => {
        let handled = false;
        opener.webContents.setWindowOpenHandler(() => {
          if (handled) return { action: 'deny' };
          handled = true;
          return { action: 'allow', overrideBrowserWindowOptions: { show: false } };
        });
        await opener.loadURL(`${url}/opener`);
        const created = once(opener.webContents, 'did-create-window') as Promise<
          [BrowserWindow, Electron.DidCreateWindowDetails]
        >;
        // A hidden window may drop input sent right after load, so click
        // until the handler has seen it.
        await waitUntil(
          () => {
            if (handled) return true;
            opener.webContents.sendInputEvent({ type: 'mouseDown', x: 10, y: 10, button: 'middle', clickCount: 1 });
            opener.webContents.sendInputEvent({ type: 'mouseUp', x: 10, y: 10, button: 'middle', clickCount: 1 });
            return false;
          },
          { rate: 100 }
        );
        const [child, details] = await created;
        if (child.webContents.isLoading()) await once(child.webContents, 'did-finish-load');
        return { child, details };
      };

      it("inherits the opener's security webPreferences", async () => {
        const opener = new BrowserWindow({ show: false, webPreferences: { javascript: false } });
        const { child, details } = await middleClickLink(opener);
        expect(details.options.webPreferences?.javascript).toBe(false);
        expect(child.webContents.getLastWebPreferences()?.javascript).toBe(false);
        expect(child.webContents.getTitle()).toBe('no-script');
      });

      it("opens in the opener's session", async () => {
        const opener = new BrowserWindow({
          show: false,
          webPreferences: { partition: 'link-opened-window' }
        });
        expect(opener.webContents.session).not.toBe(session.defaultSession);
        const { child } = await middleClickLink(opener);
        expect(child.webContents.session).toBe(opener.webContents.session);
      });
    });

    it('fires handler with correct params', async () => {
      const testFrameName = 'test-frame-name';
      const testFeatures = 'top=10&left=10&something-unknown&show=no';
      const testUrl = 'app://does-not-exist/';
      const details = await new Promise<Electron.HandlerDetails>((resolve) => {
        browserWindow.webContents.setWindowOpenHandler((details) => {
          setTimeout(() => resolve(details));
          return { action: 'deny' };
        });

        browserWindow.webContents.executeJavaScript(
          `window.open('${testUrl}', '${testFrameName}', '${testFeatures}') && true`
        );
      });
      const { url, frameName, features, disposition, referrer } = details;
      expect(url).toBe(testUrl);
      expect(frameName).toBe(testFrameName);
      expect(features).toBe(testFeatures);
      expect(disposition).toBe('new-window');
      expect(referrer).toEqual({
        policy: 'strict-origin-when-cross-origin',
        url: ''
      });
    });

    it('includes post body', async () => {
      const details = await new Promise<Electron.HandlerDetails>((resolve) => {
        browserWindow.webContents.setWindowOpenHandler((details) => {
          setTimeout(() => resolve(details));
          return { action: 'deny' };
        });

        browserWindow.webContents.loadURL(
          `data:text/html,${encodeURIComponent(`
          <form action="http://example.com" target="_blank" method="POST" id="form">
            <input name="key" value="value"></input>
          </form>
          <script>form.submit()</script>
        `)}`
        );
      });
      const { url, frameName, features, disposition, referrer, postBody } = details;
      expect(url).toBe('http://example.com/');
      expect(frameName).toBe('');
      expect(features).toEqual('');
      expect(disposition).toBe('foreground-tab');
      expect(referrer).toEqual({
        policy: 'strict-origin-when-cross-origin',
        url: ''
      });
      expect(postBody).toEqual({
        contentType: 'application/x-www-form-urlencoded',
        data: [
          {
            type: 'rawData',
            bytes: Buffer.from('key=value')
          }
        ]
      });
    });

    it('does fire window creation events if an override returns action: allow', async () => {
      browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));

      setImmediate(() => {
        browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
      });

      await once(browserWindow.webContents, 'did-create-window');
    });

    it('reuses an existing window when window.open is called with the same frame name', async () => {
      let handlerCallCount = 0;
      browserWindow.webContents.setWindowOpenHandler(() => {
        handlerCallCount++;
        return { action: 'allow' };
      });

      const didCreateWindow = once(browserWindow.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      await browserWindow.webContents.executeJavaScript(
        "window.open('about:blank?one', 'named-target', 'show=no') && true"
      );
      const [childWindow] = await didCreateWindow;
      expect(handlerCallCount).toBe(1);
      expect(childWindow.webContents.getURL()).toBe('about:blank?one');

      browserWindow.webContents.on('did-create-window', () => {
        assert.fail('did-create-window should not fire when reusing a named window');
      });

      const didNavigate = once(childWindow.webContents, 'did-navigate');
      const sameWindow = await browserWindow.webContents.executeJavaScript(`
        (() => {
          const first = window.open('about:blank?one', 'named-target', 'show=no');
          const second = window.open('about:blank?two', 'named-target', 'show=no');
          return first === second;
        })()
      `);
      await didNavigate;

      expect(sameWindow, 'window.open with matching frame name should return the same window proxy').toBe(true);
      expect(handlerCallCount, 'setWindowOpenHandler should not be called when Blink resolves the named target').toBe(
        1
      );
      expect(childWindow.webContents.getURL()).toBe('about:blank?two');
      expect(BrowserWindow.getAllWindows()).toHaveLength(2);
    });

    it('can change webPreferences of child windows', async () => {
      browserWindow.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: { webPreferences: { defaultFontSize: 30 } }
      }));

      const didCreateWindow = once(browserWindow.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
      const [childWindow] = await didCreateWindow;

      await childWindow.webContents.executeJavaScript("document.write('hello')");
      const size = await childWindow.webContents.executeJavaScript(
        "getComputedStyle(document.querySelector('body')).fontSize"
      );
      expect(size).toBe('30px');
    });

    it('does not hang parent window when denying window.open', async () => {
      browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
      browserWindow.webContents.executeJavaScript("window.open('https://127.0.0.1')");
      expect(await browserWindow.webContents.executeJavaScript('42')).toBe(42);
    });

    it('does not propagate non-allowlisted features-string options like icon to the BrowserWindow', async () => {
      browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));

      const didCreateWindow = once(browserWindow.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      browserWindow.webContents.executeJavaScript(
        "window.open('about:blank', '', 'show=no,width=400,icon=/tmp/does-not-exist.png') && true"
      );
      const [, details] = await didCreateWindow;

      expect(details.options).not.toHaveProperty('icon');
      expect(details.options.width).toBe(400);
      expect(details.options.show).toBe(false);
    });

    it('still allows the main process to set icon via overrideBrowserWindowOptions', async () => {
      const iconPath = nodePath.join(import.meta.dirname, 'fixtures', 'assets', 'icon.ico');
      browserWindow.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: { icon: iconPath }
      }));

      const didCreateWindow = once(browserWindow.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      browserWindow.webContents.executeJavaScript(
        "window.open('about:blank', '', 'show=no,icon=/tmp/attacker.png') && true"
      );
      const [, details] = await didCreateWindow;

      expect((details.options as any).icon).toBe(iconPath);
    });

    it('can open an offscreen child window from an onscreen parent', async () => {
      browserWindow.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: {
          webPreferences: {
            offscreen: true
          }
        }
      }));

      const didCreateWindow = once(browserWindow.webContents, 'did-create-window');
      const url = `file://${nodePath.join('fixtures', 'pages', 'content.html')}`;
      browserWindow.webContents.executeJavaScript(`window.open('${JSON.stringify(url)}') && true`);
      const [childWindow] = await didCreateWindow;
      expect(childWindow.webContents.isOffscreen(), 'Child window should be offscreen').toBe(true);
    });

    it('can open an onscreen child window from an offscreen parent', async () => {
      const obw = new BrowserWindow({
        show: false,
        webPreferences: {
          offscreen: true
        }
      });

      await obw.loadURL('about:blank');
      obw.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));

      const didCreateWindow = once(obw.webContents, 'did-create-window');
      const url = `file://${nodePath.join('fixtures', 'pages', 'content.html')}`;
      obw.webContents.executeJavaScript(`window.open('${JSON.stringify(url)}') && true`);
      const [childWindow] = await didCreateWindow;
      expect(childWindow.webContents.isOffscreen(), 'Child window should not be offscreen').toBe(false);
    });

    it('can open an offscreen child window from an offscreen parent', async () => {
      const obw = new BrowserWindow({
        show: false,
        webPreferences: {
          offscreen: true
        }
      });

      await obw.loadURL('about:blank');
      obw.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: {
          webPreferences: {
            offscreen: true
          }
        }
      }));

      const didCreateWindow = once(obw.webContents, 'did-create-window');
      const url = `file://${nodePath.join('fixtures', 'pages', 'content.html')}`;
      obw.webContents.executeJavaScript(`window.open('${JSON.stringify(url)}') && true`);
      const [childWindow] = await didCreateWindow;
      expect(childWindow.webContents.isOffscreen(), 'Child window should be offscreen').toBe(true);
    });

    ifit(hasCapturableScreen())(
      'should not make child window background transparent',
      { tags: ['serial'] },
      async () => {
        browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));
        const didCreateWindow = once(browserWindow.webContents, 'did-create-window');
        browserWindow.webContents.executeJavaScript("window.open('about:blank') && true");
        const [childWindow] = await didCreateWindow;
        const display = screen.getPrimaryDisplay();
        childWindow.setBounds(display.bounds);
        await childWindow.webContents.executeJavaScript(
          "const meta = document.createElement('meta'); meta.name = 'color-scheme'; meta.content = 'dark'; document.head.appendChild(meta); true;"
        );
        const capture = ScreenCapture.forWindow(childWindow);
        // color-scheme is set to dark so background should not be white
        await capture.expectColorAtCenterDoesNotMatch(HexColors.WHITE);
      }
    );
  });

  describe('custom window', () => {
    let browserWindow: BrowserWindow;

    let server: http.Server;
    let url: string;

    beforeAll(async () => {
      server = http.createServer((request, response) => {
        switch (request.url) {
          case '/index':
            response.statusCode = 200;
            response.end('<title>Index page</title>');
            break;
          case '/child':
            response.statusCode = 200;
            response.end('<title>Child page</title>');
            break;
          case '/test':
            response.statusCode = 200;
            response.end('<title>Test page</title>');
            break;
          default:
            throw new Error(`Unsupported endpoint: ${request.url}`);
        }
      });

      url = (await listen(server)).url;
    });

    afterAll(() => {
      server.close();
    });

    beforeEach(async () => {
      browserWindow = new BrowserWindow({ show: false });
      await browserWindow.loadURL(`${url}/index`);
    });

    afterEach(closeAllWindows);

    it('throws error when created window uses invalid webcontents', async () => {
      const listeners = process.listeners('uncaughtException');
      process.removeAllListeners('uncaughtException');
      const uncaughtExceptionEmitted = new Promise<void>((resolve, reject) => {
        process.on('uncaughtException', (thrown) => {
          try {
            expect(thrown.message).toBe(
              'Invalid webContents. Created window should be connected to webContents passed with options object.'
            );
            resolve();
          } catch (e) {
            reject(e);
          } finally {
            process.removeAllListeners('uncaughtException');
            listeners.forEach((listener) => process.on('uncaughtException', listener));
          }
        });
      });

      browserWindow.webContents.setWindowOpenHandler(() => {
        return {
          action: 'allow',
          createWindow: () => {
            const childWindow = new BrowserWindow({ title: 'New window' });
            return childWindow.webContents;
          }
        };
      });

      browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");

      await uncaughtExceptionEmitted;
    });

    it('spawns browser window when createWindow is provided', async () => {
      const browserWindowTitle = 'Child browser window';

      const childWindow = await new Promise<Electron.BrowserWindow>((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          return {
            action: 'allow',
            createWindow: (options) => {
              const childWindow = new BrowserWindow({ ...options, title: browserWindowTitle });
              resolve(childWindow);
              return childWindow.webContents;
            }
          };
        });

        browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
      });

      expect(childWindow.title).toBe(browserWindowTitle);
    });

    it('should be able to access the child window document when createWindow is provided', async () => {
      browserWindow.webContents.setWindowOpenHandler(() => {
        return {
          action: 'allow',
          createWindow: (options) => {
            const child = new BrowserWindow(options);
            return child.webContents;
          }
        };
      });

      const aboutBlankTitle = await browserWindow.webContents.executeJavaScript(`
        const win1 = window.open('about:blank', '', 'show=no');
        win1.document.title = 'about-blank-title';
        win1.document.title;
      `);
      expect(aboutBlankTitle).toBe('about-blank-title');

      const serverPageTitle = await browserWindow.webContents.executeJavaScript(`
        const win2 = window.open('${url}/child', '', 'show=no');
        win2.document.title = 'server-page-title';
        win2.document.title;
      `);
      expect(serverPageTitle).toBe('server-page-title');
    });

    it('spawns browser window with overridden options', async () => {
      const childWindow = await new Promise<Electron.BrowserWindow>((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          return {
            action: 'allow',
            overrideBrowserWindowOptions: {
              width: 640,
              height: 480
            },
            createWindow: (options) => {
              expect(options.width).toBe(640);
              expect(options.height).toBe(480);
              const childWindow = new BrowserWindow(options);
              resolve(childWindow);
              return childWindow.webContents;
            }
          };
        });

        browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
      });

      const size = childWindow.getSize();
      expect(size[0]).toBe(640);
      expect(size[1]).toBe(480);
    });

    it('spawns browser window with access to opener property', async () => {
      const childWindow = await new Promise<Electron.BrowserWindow>((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          return {
            action: 'allow',
            createWindow: (options) => {
              const childWindow = new BrowserWindow(options);
              resolve(childWindow);
              return childWindow.webContents;
            }
          };
        });

        browserWindow.webContents.executeJavaScript(`window.open('${url}/child', '', 'show=no') && true`);
      });

      await once(childWindow.webContents, 'ready-to-show');
      const childWindowOpenerTitle = await childWindow.webContents.executeJavaScript('window.opener.document.title');
      expect(childWindowOpenerTitle).toBe(browserWindow.title);
    });

    it('spawns browser window without access to opener property because of noopener attribute ', async () => {
      const childWindow = await new Promise<Electron.BrowserWindow>((resolve) => {
        browserWindow.webContents.setWindowOpenHandler(() => {
          return {
            action: 'allow',
            createWindow: (options) => {
              const childWindow = new BrowserWindow(options);
              resolve(childWindow);
              return childWindow.webContents;
            }
          };
        });
        browserWindow.webContents.executeJavaScript(`window.open('${url}/child', '', 'noopener,show=no') && true`);
      });

      await once(childWindow.webContents, 'ready-to-show');
      await expect(childWindow.webContents.executeJavaScript('window.opener.document.title')).rejects.toThrow(
        'Script failed to execute, this normally means an error was thrown. Check the renderer console for the error.'
      );
    });
  });
});
