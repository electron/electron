import { expect } from 'chai';
import * as path from 'path';

import { BrowserWindow, WebPreferences, app } from 'electron/main';
import { closeWindow } from './window-helpers';

describe('BrowserWindow with affinity module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');
  const myAffinityName = 'myAffinity';
  const myAffinityNameUpper = 'MYAFFINITY';
  const anotherAffinityName = 'anotherAffinity';

  before(() => {
    app.allowRendererProcessReuse = false;
  });

  after(() => {
    app.allowRendererProcessReuse = true;
  });

  async function createWindowWithWebPrefs (webPrefs: WebPreferences) {
    const w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: webPrefs || {}
    });
    await w.loadFile(path.join(fixtures, 'api', 'blank.html'));
    return w;
  }

  function testAffinityProcessIds (name: string, webPreferences: WebPreferences = {}) {
    describe(name, () => {
      let mAffinityWindow: BrowserWindow;
      before(async () => {
        mAffinityWindow = await createWindowWithWebPrefs({ affinity: myAffinityName, ...webPreferences });
      });

      after(async () => {
        await closeWindow(mAffinityWindow, { assertNotWindows: false });
        mAffinityWindow = null as unknown as BrowserWindow;
      });

      it('should have a different process id than a default window', async () => {
        const w = await createWindowWithWebPrefs({ ...webPreferences });
        const affinityID = mAffinityWindow.webContents.getOSProcessId();
        const wcID = w.webContents.getOSProcessId();

        expect(affinityID).to.not.equal(wcID, 'Should have different OS process IDs');
        await closeWindow(w, { assertNotWindows: false });
      });

      it(`should have a different process id than a window with a different affinity '${anotherAffinityName}'`, async () => {
        const w = await createWindowWithWebPrefs({ affinity: anotherAffinityName, ...webPreferences });
        const affinityID = mAffinityWindow.webContents.getOSProcessId();
        const wcID = w.webContents.getOSProcessId();

        expect(affinityID).to.not.equal(wcID, 'Should have different OS process IDs');
        await closeWindow(w, { assertNotWindows: false });
      });

      it(`should have the same OS process id than a window with the same affinity '${myAffinityName}'`, async () => {
        const w = await createWindowWithWebPrefs({ affinity: myAffinityName, ...webPreferences });
        const affinityID = mAffinityWindow.webContents.getOSProcessId();
        const wcID = w.webContents.getOSProcessId();

        expect(affinityID).to.equal(wcID, 'Should have the same OS process ID');
        await closeWindow(w, { assertNotWindows: false });
      });

      it(`should have the same OS process id than a window with an equivalent affinity '${myAffinityNameUpper}' (case insensitive)`, async () => {
        const w = await createWindowWithWebPrefs({ affinity: myAffinityNameUpper, ...webPreferences });
        const affinityID = mAffinityWindow.webContents.getOSProcessId();
        const wcID = w.webContents.getOSProcessId();

        expect(affinityID).to.equal(wcID, 'Should have the same OS process ID');
        await closeWindow(w, { assertNotWindows: false });
      });
    });
  }

  testAffinityProcessIds(`BrowserWindow with an affinity '${myAffinityName}'`);
  testAffinityProcessIds(`BrowserWindow with an affinity '${myAffinityName}' and sandbox enabled`, { sandbox: true });
  testAffinityProcessIds(`BrowserWindow with an affinity '${myAffinityName}' and nativeWindowOpen enabled`, { nativeWindowOpen: true });
});
