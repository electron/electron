import { BrowserWindow } from 'electron';
import { writeFileSync, readFileSync } from 'fs';
import { resolve } from 'path';
import { expect } from 'chai';
import { closeAllWindows } from './window-helpers';

function genSnapshot (browserWindow: BrowserWindow, features: string) {
  return new Promise((resolve) => {
    browserWindow.webContents.on('new-window', (...args: any[]) => {
      resolve([features, ...args]);
    });
    browserWindow.webContents.executeJavaScript(`window.open('about:blank', 'frame name', '${features}')`);
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
        browserWindow.on('ready-to-show', () => done());
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

function stringifySnapshots (snapshots: any, pretty = false) {
  return JSON.stringify(snapshots, (key, value) => {
    if (['sender', 'webContents'].includes(key)) {
      return '[WebContents]';
    }
    if (key === 'openerId' && typeof value === 'number') {
      return 'placeholder-opener-id';
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
