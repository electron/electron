import { expect } from 'chai';
import { BrowserWindow } from 'electron/main';
import * as path from 'node:path';
import { closeAllWindows } from './lib/window-helpers';
import { setTimeout } from 'node:timers/promises';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('Long Animation Frame (LoAF) API', () => {
  afterEach(closeAllWindows);

  describe('script attribution for file:// URLs', () => {
    it('should include script attribution when loading from file:// URL', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true
        }
      });

      await w.loadFile(path.join(fixturesPath, 'pages', 'loaf-test.html'));

      // Trigger a long animation frame by simulating a click
      await w.webContents.executeJavaScript(`
        return new Promise((resolve) => {
          const button = document.getElementById('block');
          button.click();
          // Wait a bit for the LoAF entry to be recorded
          setTimeout(resolve, 300);
        });
      `);

      // Get the recorded LoAF entries
      const entries = await w.webContents.executeJavaScript('window.loafEntries');

      // Verify we got at least one LoAF entry
      expect(entries).to.be.an('array');
      expect(entries.length).to.be.greaterThan(0, 'should have at least one LoAF entry');

      // Find an entry with scripts
      const entryWithScripts = entries.find((entry: any) => entry.scripts && entry.scripts.length > 0);

      // With the patch applied, we should have script attribution even for file:// URLs
      expect(entryWithScripts).to.not.be.undefined;
      if (entryWithScripts) {
        expect(entryWithScripts.scripts).to.be.an('array');
        expect(entryWithScripts.scripts.length).to.be.greaterThan(0, 'should have script attribution');

        const script = entryWithScripts.scripts[0];
        expect(script).to.have.property('sourceURL');
        expect(script.sourceURL).to.include('loaf-test.html');
        expect(script).to.have.property('sourceFunctionName');
        expect(script.sourceFunctionName).to.equal('block');
      }
    });

    it('should include sourceCharPosition in script attribution', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true
        }
      });

      await w.loadFile(path.join(fixturesPath, 'pages', 'loaf-test.html'));

      // Trigger a long animation frame
      await w.webContents.executeJavaScript(`
        return new Promise((resolve) => {
          document.getElementById('block').click();
          setTimeout(resolve, 300);
        });
      `);

      const entries = await w.webContents.executeJavaScript('window.loafEntries');
      const entryWithScripts = entries.find((entry: any) => entry.scripts && entry.scripts.length > 0);

      if (entryWithScripts && entryWithScripts.scripts.length > 0) {
        const script = entryWithScripts.scripts[0];
        expect(script).to.have.property('sourceCharPosition');
        expect(typeof script.sourceCharPosition).to.equal('number');
      }
    });

    it('should capture script timing information', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true
        }
      });

      await w.loadFile(path.join(fixturesPath, 'pages', 'loaf-test.html'));

      await w.webContents.executeJavaScript(`
        return new Promise((resolve) => {
          document.getElementById('block').click();
          setTimeout(resolve, 300);
        });
      `);

      const entries = await w.webContents.executeJavaScript('window.loafEntries');

      expect(entries.length).to.be.greaterThan(0);

      const entry = entries[0];
      // Verify LoAF timing properties are present
      expect(entry).to.have.property('startTime');
      expect(entry).to.have.property('duration');
      expect(entry).to.have.property('blockingDuration');

      // Duration should be > 50ms since we blocked for 200ms
      expect(entry.duration).to.be.greaterThan(50);
    });
  });
});
