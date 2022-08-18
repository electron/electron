import { BrowserWindow } from 'electron';
import { expect, assert } from 'chai';
import { closeAllWindows } from './window-helpers';
const { emittedOnce } = require('./events-helpers');

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
        listeners.forEach((listener) => process.on('uncaughtException', listener));
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

    await emittedOnce(browserWindow.webContents, 'did-create-window');
  });

  it('can change webPreferences of child windows', (done) => {
    browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow', overrideBrowserWindowOptions: { webPreferences: { defaultFontSize: 30 } } }));

    browserWindow.webContents.on('did-create-window', async (childWindow) => {
      await childWindow.webContents.executeJavaScript("document.write('hello')");
      const size = await childWindow.webContents.executeJavaScript("getComputedStyle(document.querySelector('body')).fontSize");
      expect(size).to.equal('30px');
      done();
    });

    browserWindow.webContents.executeJavaScript("window.open('about:blank', '', 'show=no') && true");
  });

  it('does not hang parent window when denying window.open', async () => {
    browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
    browserWindow.webContents.executeJavaScript("window.open('https://127.0.0.1')");
    expect(await browserWindow.webContents.executeJavaScript('42')).to.equal(42);
  });
});
