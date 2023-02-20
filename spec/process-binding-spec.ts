import { expect } from 'chai';
import { BrowserWindow } from 'electron/main';
import { closeAllWindows } from './lib/window-helpers';

describe('process._linkedBinding', () => {
  describe('in the main process', () => {
    it('can access electron_browser bindings', () => {
      process._linkedBinding('electron_browser_app');
    });

    it('can access electron_common bindings', () => {
      process._linkedBinding('electron_common_v8_util');
    });

    it('cannot access electron_renderer bindings', () => {
      expect(() => {
        process._linkedBinding('electron_renderer_ipc');
      }).to.throw(/No such binding was linked: electron_renderer_ipc/);
    });
  });

  describe('in the renderer process', () => {
    afterEach(closeAllWindows);

    it('cannot access electron_browser bindings', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      await expect(w.webContents.executeJavaScript('void process._linkedBinding(\'electron_browser_app\')'))
        .to.eventually.be.rejectedWith(/Script failed to execute/);
    });

    it('can access electron_common bindings', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      await w.webContents.executeJavaScript('void process._linkedBinding(\'electron_common_v8_util\')');
    });

    it('can access electron_renderer bindings', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      await w.webContents.executeJavaScript('void process._linkedBinding(\'electron_renderer_ipc\')');
    });
  });
});
