import { BrowserWindow, webContents, ipcMain, WebContents } from 'electron/main';
import { expect } from 'chai';
import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import { closeAllWindows } from './lib/window-helpers';

describe('stability fixes', () => {
  afterEach(closeAllWindows);

  describe('printToPDF deadlock fix', () => {
    it('does not deadlock after a failed print job', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      // Trigger a guaranteed failure (invalid margins)
      await expect(w.webContents.printToPDF({
        margins: { top: 100, bottom: 100, left: 100, right: 100 } // Exceeds page size
      })).to.eventually.be.rejected();

      // Subsequent call should work without deadlocking
      const promise = w.webContents.printToPDF({});
      const result = await Promise.race([
        promise,
        setTimeout(5000).then(() => 'timeout')
      ]);

      expect(result).to.not.equal('timeout');
      expect(Buffer.isBuffer(result)).to.be.true();
    });

    it('does not affect other windows after a failure', async () => {
      const w1 = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({ show: false });
      await Promise.all([w1.loadURL('about:blank'), w2.loadURL('about:blank')]);

      // Fail in window 1
      await expect(w1.webContents.printToPDF({
        margins: { top: 100, bottom: 100, left: 100, right: 100 }
      })).to.eventually.be.rejected();

      // Window 2 should still be able to print
      const result = await w2.webContents.printToPDF({});
      expect(Buffer.isBuffer(result)).to.be.true();
    });
  });

  describe('C++ DidFinishNavigation re-entrancy fix', () => {
    it('does not crash when window is destroyed during did-navigate', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.on('did-navigate', () => {
        w.destroy();
      });

      // Navigate to trigger the event and potential C++ UAF
      await w.loadURL('data:text/html,<h1>Test</h1>');
      
      // If we didn't crash, the test passes
      expect(w.isDestroyed()).to.be.true();
    });

    it('does not crash when window is destroyed during did-frame-navigate', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.on('did-frame-navigate', () => {
        w.destroy();
      });

      await w.loadURL('data:text/html,<h1>Test</h1>');
      expect(w.isDestroyed()).to.be.true();
    });
  });

  describe('Internal IPC listener leak fix', () => {
    it('cleans up response listeners when WebContents is destroyed', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      // ipcMainInternal is not exported but we can check it indirectly 
      // by inspecting listeners on the internal channels if we had access.
      // Since it's internal, we'll verify that calling executeJavaScript 
      // doesn't leave dangling references to destroyed windows.
      
      const ipcMainInternal = (process as any)._linkedBinding('electron_browser_ipc').ipcMainInternal;
      const initialListeners = ipcMainInternal.eventNames().length;
      
      // Start a request that will stay "pending" for a bit
      w.webContents.executeJavaScript('new Promise(() => {})'); 
      
      // Destroy the window mid-request
      w.destroy();
      
      // Wait a tick for cleanup
      await setTimeout(100);
      
      const finalListeners = ipcMainInternal.eventNames().length;
      // Should have cleaned up the unique channel listener
      expect(finalListeners).to.be.lessThanOrEqual(initialListeners);
    });
  });
});
