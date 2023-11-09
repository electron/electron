import { BrowserWindow, screen } from 'electron';
import { expect, assert } from 'chai';
import { areColorsSimilar, captureScreen, HexColors, getPixelColor } from './lib/screen-helpers';
import { ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';
import { once } from 'node:events';
import { setTimeout as setTimeoutAsync } from 'node:timers/promises';

describe('webContents.setWindowOpenHandler', () => {
  let browserWindow: BrowserWindow;
  beforeEach(async () => {
    browserWindow = new BrowserWindow({ show: false });
    await browserWindow.loadURL('about:blank');
  });

  afterEach(closeAllWindows);

  it('does not fire window creation events if the handler callback throws an error', (done) => {
    const error = new Error('oh no');
    const listeners = process.listeners('uncaughtException');
    process.removeAllListeners('uncaughtException');
    process.on('uncaughtException', (thrown) => {
      try {
        expect(thrown).to.equal(error);
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
  });

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

    await browserWindow.webContents.loadURL('data:text/html,<a target="_blank" href="http://example.com" style="display: block; width: 100%; height: 100%; position: fixed; left: 0; top: 0;">link</a>');
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

    await browserWindow.webContents.loadURL('data:text/html,<a href="http://example.com" style="display: block; width: 100%; height: 100%; position: fixed; left: 0; top: 0;">link</a>');
    browserWindow.webContents.sendInputEvent({ type: 'mouseDown', x: 10, y: 10, button: 'left', clickCount: 1, modifiers: ['shift'] });
    browserWindow.webContents.sendInputEvent({ type: 'mouseUp', x: 10, y: 10, button: 'left', clickCount: 1, modifiers: ['shift'] });

    await denied;
  });

  it('fires handler with correct params', async () => {
    const testFrameName = 'test-frame-name';
    const testFeatures = 'top=10&left=10&something-unknown&show=no';
    const testUrl = 'app://does-not-exist/';
    const details = await new Promise<Electron.HandlerDetails>(resolve => {
      browserWindow.webContents.setWindowOpenHandler((details) => {
        setTimeout(() => resolve(details));
        return { action: 'deny' };
      });

      browserWindow.webContents.executeJavaScript(`window.open('${testUrl}', '${testFrameName}', '${testFeatures}') && true`);
    });
    const { url, frameName, features, disposition, referrer } = details;
    expect(url).to.equal(testUrl);
    expect(frameName).to.equal(testFrameName);
    expect(features).to.equal(testFeatures);
    expect(disposition).to.equal('new-window');
    expect(referrer).to.deep.equal({
      policy: 'strict-origin-when-cross-origin',
      url: ''
    });
  });

  it('includes post body', async () => {
    const details = await new Promise<Electron.HandlerDetails>(resolve => {
      browserWindow.webContents.setWindowOpenHandler((details) => {
        setTimeout(() => resolve(details));
        return { action: 'deny' };
      });

      browserWindow.webContents.loadURL(`data:text/html,${encodeURIComponent(`
        <form action="http://example.com" target="_blank" method="POST" id="form">
          <input name="key" value="value"></input>
        </form>
        <script>form.submit()</script>
      `)}`);
    });
    const { url, frameName, features, disposition, referrer, postBody } = details;
    expect(url).to.equal('http://example.com/');
    expect(frameName).to.equal('');
    expect(features).to.deep.equal('');
    expect(disposition).to.equal('foreground-tab');
    expect(referrer).to.deep.equal({
      policy: 'strict-origin-when-cross-origin',
      url: ''
    });
    expect(postBody).to.deep.equal({
      contentType: 'application/x-www-form-urlencoded',
      data: [{
        type: 'rawData',
        bytes: Buffer.from('key=value')
      }]
    });
  });

  it('does fire window creation events if an override returns action: allow', async () => {
    browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));

    setImmediate(() => {
      browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
    });

    await once(browserWindow.webContents, 'did-create-window');
  });

  it('can change webPreferences of child windows', async () => {
    browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow', overrideBrowserWindowOptions: { webPreferences: { defaultFontSize: 30 } } }));

    const didCreateWindow = once(browserWindow.webContents, 'did-create-window') as Promise<[BrowserWindow, Electron.DidCreateWindowDetails]>;
    browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
    const [childWindow] = await didCreateWindow;

    await childWindow.webContents.executeJavaScript("document.write('hello')");
    const size = await childWindow.webContents.executeJavaScript("getComputedStyle(document.querySelector('body')).fontSize");
    expect(size).to.equal('30px');
  });

  it('does not hang parent window when denying window.open', async () => {
    browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
    browserWindow.webContents.executeJavaScript("window.open('https://127.0.0.1')");
    expect(await browserWindow.webContents.executeJavaScript('42')).to.equal(42);
  });

  // Linux and arm64 platforms (WOA and macOS) do not return any capture sources
  ifit(process.platform === 'darwin' && process.arch === 'x64')('should not make child window background transparent', async () => {
    browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));
    const didCreateWindow = once(browserWindow.webContents, 'did-create-window');
    browserWindow.webContents.executeJavaScript("window.open('about:blank') && true");
    const [childWindow] = await didCreateWindow;
    const display = screen.getPrimaryDisplay();
    childWindow.setBounds(display.bounds);
    await childWindow.webContents.executeJavaScript("const meta = document.createElement('meta'); meta.name = 'color-scheme'; meta.content = 'dark'; document.head.appendChild(meta); true;");
    await setTimeoutAsync(1000);
    const screenCapture = await captureScreen();
    const centerColor = getPixelColor(screenCapture, {
      x: display.size.width / 2,
      y: display.size.height / 2
    });
    // color-scheme is set to dark so background should not be white
    expect(areColorsSimilar(centerColor, HexColors.WHITE)).to.be.false();
  });
});
