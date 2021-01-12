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
    browserWindow.webContents.executeJavaScript(`window.open('about:blank', 'frame name', '${features}') && true`);
  });
}

describe('new-window event', () => {
  const testConfig = {
    native: {
      snapshotFileName: 'native-window-open.snapshot.txt',
      browserWindowOptions: {
        show: false,
        width: 200,
        title: 'cool',
        backgroundColor: 'blue',
        focusable: false,
        webPreferences: {
          nativeWindowOpen: true,
          sandbox: true
        }
      }
    },
    proxy: {
      snapshotFileName: 'proxy-window-open.snapshot.txt',
      browserWindowOptions: {
        show: false
      }
    }
  };

  for (const testName of Object.keys(testConfig) as (keyof typeof testConfig)[]) {
    const { snapshotFileName, browserWindowOptions } = testConfig[testName];

    describe(`for ${testName} window opening`, () => {
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
  }
});

describe('webContents.setWindowOpenHandler', () => {
  const testConfig = {
    native: {
      browserWindowOptions: {
        show: false,
        webPreferences: {
          nativeWindowOpen: true
        }
      }
    },
    proxy: {
      browserWindowOptions: {
        show: false,
        webPreferences: {
          nativeWindowOpen: false
        }
      }
    }
  };

  for (const testName of Object.keys(testConfig) as (keyof typeof testConfig)[]) {
    let browserWindow: BrowserWindow;
    const { browserWindowOptions } = testConfig[testName];

    describe(testName, () => {
      beforeEach((done) => {
        browserWindow = new BrowserWindow(browserWindowOptions);
        browserWindow.loadURL('about:blank');
        browserWindow.on('ready-to-show', () => { browserWindow.show(); done(); });
      });

      afterEach(closeAllWindows);

      it('does not fire window creation events if an override returns action: deny', (done) => {
        browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
        browserWindow.webContents.on('new-window', () => {
          assert.fail('new-window should not to be called with an overridden window.open');
        });

        browserWindow.webContents.on('did-create-window', () => {
          assert.fail('did-create-window should not to be called with an overridden window.open');
        });

        browserWindow.webContents.executeJavaScript("window.open('about:blank') && true");

        setTimeout(() => {
          done();
        }, 500);
      });

      it('fires handler with correct params', (done) => {
        const testFrameName = 'test-frame-name';
        const testFeatures = 'top=10&left=10&something-unknown';
        const testUrl = 'app://does-not-exist/';
        browserWindow.webContents.setWindowOpenHandler(({ url, frameName, features }) => {
          expect(url).to.equal(testUrl);
          expect(frameName).to.equal(testFrameName);
          expect(features).to.equal(testFeatures);
          done();
          return { action: 'deny' };
        });

        browserWindow.webContents.executeJavaScript(`window.open('${testUrl}', '${testFrameName}', '${testFeatures}') && true`);
      });

      it('does fire window creation events if an override returns action: allow', async () => {
        browserWindow.webContents.setWindowOpenHandler(() => ({ action: 'allow' }));

        setImmediate(() => {
          browserWindow.webContents.executeJavaScript("window.open('about:blank') && true");
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

        browserWindow.webContents.executeJavaScript("window.open('about:blank') && true");
      });
    });
  }
});

function stringifySnapshots (snapshots: any, pretty = false) {
  return JSON.stringify(snapshots, (key, value) => {
    if (['sender', 'webContents'].includes(key)) {
      return '[WebContents]';
    }
    if (key === 'openerId' && typeof value === 'number') {
      return 'placeholder-opener-id';
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
  return JSON.parse(snapshotsJson, (key, value) => {
    if (key === 'openerId' && value === 'placeholder-opener-id') return 1;
    return value;
  });
}
