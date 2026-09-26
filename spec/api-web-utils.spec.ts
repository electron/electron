import { BrowserWindow } from 'electron/main';

import { expect } from 'vitest';

import * as path from 'node:path';

import { defer } from './lib/spec-helpers.ts';

// import { once } from 'node:events';

describe('webUtils module', () => {
  const fixtures = path.resolve(import.meta.dirname, 'fixtures');

  describe('getPathForFile', () => {
    it('returns nothing for a Blob', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: false,
          nodeIntegration: true,
          sandbox: false
        }
      });
      defer(() => w.close());
      await w.loadFile(path.resolve(fixtures, 'pages', 'file-input.html'));
      const pathFromWebUtils = await w.webContents.executeJavaScript(
        'require("electron").webUtils.getPathForFile(new Blob([1, 2, 3]))'
      );
      expect(pathFromWebUtils).to.equal('');
    });

    it('reports the correct path for a File object', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: false,
          nodeIntegration: true,
          sandbox: false
        }
      });
      defer(() => w.close());
      await w.loadFile(path.resolve(fixtures, 'pages', 'file-input.html'));
      const { debugger: debug } = w.webContents;
      debug.attach();
      try {
        const {
          root: { nodeId }
        } = await debug.sendCommand('DOM.getDocument');
        const { nodeId: inputNodeId } = await debug.sendCommand('DOM.querySelector', { nodeId, selector: 'input' });
        await debug.sendCommand('DOM.setFileInputFiles', {
          files: [import.meta.filename],
          nodeId: inputNodeId
        });
        const pathFromWebUtils = await w.webContents.executeJavaScript(
          'require("electron").webUtils.getPathForFile(document.querySelector("input").files[0])'
        );
        expect(pathFromWebUtils).to.equal(import.meta.filename);
      } finally {
        debug.detach();
      }
    });
  });
});
