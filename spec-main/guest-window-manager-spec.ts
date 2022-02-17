import { BrowserWindow } from 'electron';
import { writeFileSync, readFileSync } from 'fs';
import { resolve } from 'path';
import { expect, assert } from 'chai';
import { closeAllWindows } from './window-helpers';
const { emittedOnce } = require('./events-helpers');

function genSnapshot (browserWindow: BrowserWindow, features: string) {
  return new Promise((resolve) => {
    browserWindow.webContents.on('new-window', (...args: any[]) => {
      resolve([features, ...args]);
    });
    browserWindow.webContents.executeJavaScript(`window.open('about:blank', 'frame-name', '${features}') && true`);
  });
}

describe('new-window event', () => {
  const snapshotFileName = 'native-window-open.snapshot.txt';
  const browserWindowOptions = {
    show: false,
    width: 200,
    title: 'cool',
    backgroundColor: 'blue',
    focusable: false,
    webPreferences: {
      sandbox: true
    }
  };

  const snapshotFile = resolve(__dirname, 'fixtures', 'snapshots', snapshotFileName);
  let browserWindow: BrowserWindow;
  let existingSnapshots: any[];

  before(() => {
    existingSnapshots = parseSnapshots(readFileSync(snapshotFile, { encoding: 'utf8' }));
  });

  beforeEach((done) => {
    browserWindow = new BrowserWindow(browserWindowOptions);
    browserWindow.loadURL('about:blank');
    browserWindow.on('ready-to-show', () => { done(); });
  });

  afterEach(closeAllWindows);

  const newSnapshots: any[] = [];
  [
    'top=5,left=10,resizable=no',
    'zoomFactor=2,resizable=0,x=0,y=10',
    'backgroundColor=gray,webPreferences=0,x=100,y=100',
    'x=50,y=20,title=sup',
    'show=false,top=1,left=1'
  ].forEach((features, index) => {
    /**
     * ATTN: If this test is failing, you likely just need to change
     * `shouldOverwriteSnapshot` to true and then evaluate the snapshot diff
     * to see if the change is harmless.
     */
    it(`matches snapshot for ${features}`, async () => {
      const newSnapshot = await genSnapshot(browserWindow, features);
      newSnapshots.push(newSnapshot);
      // TODO: The output when these fail could be friendlier.
      expect(stringifySnapshots(newSnapshot)).to.equal(stringifySnapshots(existingSnapshots[index]));
    });
  });

  after(() => {
    const shouldOverwriteSnapshot = false;
    if (shouldOverwriteSnapshot) writeFileSync(snapshotFile, stringifySnapshots(newSnapshots, true));
  });
});

describe('webContents.setWindowOpenHandler', () => {
  let browserWindow: BrowserWindow;
  beforeEach(async () => {
    browserWindow = new BrowserWindow({ show: false });
    await browserWindow.loadURL('about:blank');
  });

  afterEach(closeAllWindows);

  it('does not fire window creation events if an override returns action: deny', async () => {
    const denied = new Promise((resolve) => {
      browserWindow.webContents.setWindowOpenHandler(() => {
        setTimeout(resolve);
        return { action: 'deny' };
      });
    });
    browserWindow.webContents.on('new-window', () => {
      assert.fail('new-window should not to be called with an overridden window.open');
    });

    browserWindow.webContents.on('did-create-window', () => {
      assert.fail('did-create-window should not to be called with an overridden window.open');
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
    browserWindow.webContents.on('new-window', () => {
      assert.fail('new-window should not to be called with an overridden window.open');
    });

    browserWindow.webContents.on('did-create-window', () => {
      assert.fail('did-create-window should not to be called with an overridden window.open');
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
    browserWindow.webContents.on('new-window', () => {
      assert.fail('new-window should not to be called with an overridden window.open');
    });

    browserWindow.webContents.on('did-create-window', () => {
      assert.fail('did-create-window should not to be called with an overridden window.open');
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

    await Promise.all([
      emittedOnce(browserWindow.webContents, 'did-create-window'),
      emittedOnce(browserWindow.webContents, 'new-window')
    ]);
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

function stringifySnapshots (snapshots: any, pretty = false) {
  return JSON.stringify(snapshots, (key, value) => {
    if (['sender', 'webContents'].includes(key)) {
      return '[WebContents]';
    }
    if (key === 'processId' && typeof value === 'number') {
      return 'placeholder-process-id';
    }
    if (key === 'returnValue') {
      return 'placeholder-guest-contents-id';
    }
    return value;
  }, pretty ? 2 : undefined);
}

function parseSnapshots (snapshotsJson: string) {
  return JSON.parse(snapshotsJson);
}
