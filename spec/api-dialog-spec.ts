import { dialog, BaseWindow, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

describe('dialog module', () => {
  describe('showOpenDialog', () => {
    afterEach(closeAllWindows);
    ifit(process.platform !== 'win32')('should not throw for valid cases', () => {
      expect(() => {
        dialog.showOpenDialog({ title: 'i am title' });
      }).to.not.throw();

      expect(() => {
        const w = new BrowserWindow();
        dialog.showOpenDialog(w, { title: 'i am title' });
      }).to.not.throw();

      expect(() => {
        const w = new BaseWindow();
        dialog.showOpenDialog(w, { title: 'i am title' });
      }).to.not.throw();
    });

    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showOpenDialog({ properties: false as any });
      }).to.throw(/Properties must be an array/);

      expect(() => {
        dialog.showOpenDialog({ title: 300 as any });
      }).to.throw(/Title must be a string/);

      expect(() => {
        dialog.showOpenDialog({ buttonLabel: [] as any });
      }).to.throw(/Button label must be a string/);

      expect(() => {
        dialog.showOpenDialog({ defaultPath: {} as any });
      }).to.throw(/Default path must be a string/);

      expect(() => {
        dialog.showOpenDialog({ message: {} as any });
      }).to.throw(/Message must be a string/);
    });
  });

  describe('showSaveDialog', () => {
    afterEach(closeAllWindows);
    ifit(process.platform !== 'win32')('should not throw for valid cases', () => {
      expect(() => {
        dialog.showSaveDialog({ title: 'i am title' });
      }).to.not.throw();

      expect(() => {
        const w = new BrowserWindow();
        dialog.showSaveDialog(w, { title: 'i am title' });
      }).to.not.throw();

      expect(() => {
        const w = new BaseWindow();
        dialog.showSaveDialog(w, { title: 'i am title' });
      }).to.not.throw();
    });

    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showSaveDialog({ title: 300 as any });
      }).to.throw(/Title must be a string/);

      expect(() => {
        dialog.showSaveDialog({ buttonLabel: [] as any });
      }).to.throw(/Button label must be a string/);

      expect(() => {
        dialog.showSaveDialog({ defaultPath: {} as any });
      }).to.throw(/Default path must be a string/);

      expect(() => {
        dialog.showSaveDialog({ message: {} as any });
      }).to.throw(/Message must be a string/);

      expect(() => {
        dialog.showSaveDialog({ nameFieldLabel: {} as any });
      }).to.throw(/Name field label must be a string/);
    });
  });

  describe('showMessageBox', () => {
    afterEach(closeAllWindows);

    // parentless message boxes are synchronous on macOS
    // dangling message boxes on windows cause a DCHECK: https://source.chromium.org/chromium/chromium/src/+/main:base/win/message_window.cc;drc=7faa4bf236a866d007dc5672c9ce42660e67a6a6;l=68
    ifit(process.platform !== 'darwin' && process.platform !== 'win32')('should not throw for a parentless message box', () => {
      expect(() => {
        dialog.showMessageBox({ message: 'i am message' });
      }).to.not.throw();
    });

    ifit(process.platform !== 'win32')('should not throw for valid cases', () => {
      expect(() => {
        const w = new BrowserWindow();
        dialog.showMessageBox(w, { message: 'i am message' });
      }).to.not.throw();

      expect(() => {
        const w = new BaseWindow();
        dialog.showMessageBox(w, { message: 'i am message' });
      }).to.not.throw();
    });

    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showMessageBox(undefined as any, { type: 'not-a-valid-type' as any, message: '' });
      }).to.throw(/Invalid message box type/);

      expect(() => {
        dialog.showMessageBox(null as any, { buttons: false as any, message: '' });
      }).to.throw(/Buttons must be an array/);

      expect(() => {
        dialog.showMessageBox({ title: 300 as any, message: '' });
      }).to.throw(/Title must be a string/);

      expect(() => {
        dialog.showMessageBox({ message: [] as any });
      }).to.throw(/Message must be a string/);

      expect(() => {
        dialog.showMessageBox({ detail: 3.14 as any, message: '' });
      }).to.throw(/Detail must be a string/);

      expect(() => {
        dialog.showMessageBox({ checkboxLabel: false as any, message: '' });
      }).to.throw(/checkboxLabel must be a string/);
    });
  });

  describe('showMessageBox with signal', () => {
    afterEach(closeAllWindows);

    it('closes message box immediately', async () => {
      const controller = new AbortController();
      const signal = controller.signal;
      const w = new BrowserWindow();
      const p = dialog.showMessageBox(w, { signal, message: 'i am message' });
      controller.abort();
      const result = await p;
      expect(result.response).to.equal(0);
    });

    it('closes message box after a while', async () => {
      const controller = new AbortController();
      const signal = controller.signal;
      const w = new BrowserWindow();
      const p = dialog.showMessageBox(w, { signal, message: 'i am message' });
      await setTimeout(500);
      controller.abort();
      const result = await p;
      expect(result.response).to.equal(0);
    });

    it('does not crash when there is a defaultId but no buttons', async () => {
      const controller = new AbortController();
      const signal = controller.signal;
      const w = new BrowserWindow();
      const p = dialog.showMessageBox(w, {
        signal,
        message: 'i am message',
        type: 'info',
        defaultId: 0,
        title: 'i am title'
      });
      controller.abort();
      const result = await p;
      expect(result.response).to.equal(0);
    });

    it('cancels message box', async () => {
      const controller = new AbortController();
      const signal = controller.signal;
      const w = new BrowserWindow();
      const p = dialog.showMessageBox(w, {
        signal,
        message: 'i am message',
        buttons: ['OK', 'Cancel'],
        cancelId: 1
      });
      controller.abort();
      const result = await p;
      expect(result.response).to.equal(1);
    });

    it('cancels message box after a while', async () => {
      const controller = new AbortController();
      const signal = controller.signal;
      const w = new BrowserWindow();
      const p = dialog.showMessageBox(w, {
        signal,
        message: 'i am message',
        buttons: ['OK', 'Cancel'],
        cancelId: 1
      });
      await setTimeout(500);
      controller.abort();
      const result = await p;
      expect(result.response).to.equal(1);
    });
  });

  describe('showErrorBox', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        (dialog.showErrorBox as any)();
      }).to.throw(/Insufficient number of arguments/);

      expect(() => {
        dialog.showErrorBox(3 as any, 'four');
      }).to.throw(/Error processing argument at index 0/);

      expect(() => {
        dialog.showErrorBox('three', 4 as any);
      }).to.throw(/Error processing argument at index 1/);
    });
  });

  describe('showCertificateTrustDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        (dialog.showCertificateTrustDialog as any)();
      }).to.throw(/options must be an object/);

      expect(() => {
        dialog.showCertificateTrustDialog({} as any);
      }).to.throw(/certificate must be an object/);

      expect(() => {
        dialog.showCertificateTrustDialog({ certificate: {} as any, message: false as any });
      }).to.throw(/message must be a string/);
    });
  });

  ifdescribe(process.platform === 'darwin' && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)('end-to-end dialog interaction (macOS)', () => {
    let dialogHelper: any;

    before(() => {
      dialogHelper = require('@electron-ci/dialog-helper');
    });

    afterEach(closeAllWindows);

    // Poll for a sheet to appear on the given window.
    async function waitForSheet (w: BrowserWindow): Promise<void> {
      const handle = w.getNativeWindowHandle();
      for (let i = 0; i < 50; i++) {
        const info = dialogHelper.getDialogInfo(handle);
        if (info.type !== 'none') return;
        await setTimeout(100);
      }
      throw new Error('Timed out waiting for dialog sheet to appear');
    }

    describe('showMessageBox', () => {
      it('shows the correct message and buttons', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Test message',
          buttons: ['OK', 'Cancel']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.type).to.equal('message-box');
        expect(info.message).to.equal('Test message');

        const buttons = JSON.parse(info.buttons);
        expect(buttons).to.include('OK');
        expect(buttons).to.include('Cancel');

        dialogHelper.clickMessageBoxButton(handle, 0);
        await p;
      });

      it('shows detail text', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Main message',
          detail: 'Extra detail text',
          buttons: ['OK']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.message).to.equal('Main message');
        expect(info.detail).to.equal('Extra detail text');

        dialogHelper.clickMessageBoxButton(handle, 0);
        await p;
      });

      it('returns the correct response when a specific button is clicked', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Choose a button',
          buttons: ['First', 'Second', 'Third']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        dialogHelper.clickMessageBoxButton(handle, 1);

        const result = await p;
        expect(result.response).to.equal(1);
      });

      it('returns the correct response when the last button is clicked', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Choose a button',
          buttons: ['Yes', 'No', 'Maybe']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        dialogHelper.clickMessageBoxButton(handle, 2);

        const result = await p;
        expect(result.response).to.equal(2);
      });

      it('shows a single button when no buttons are specified', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'No buttons specified'
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.type).to.equal('message-box');
        // macOS adds a default "OK" button when none are specified.
        const buttons = JSON.parse(info.buttons);
        expect(buttons).to.have.lengthOf(1);

        dialogHelper.clickMessageBoxButton(handle, 0);
        const result = await p;
        expect(result.response).to.equal(0);
      });

      it('renders checkbox with the correct label and initial state', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Checkbox test',
          buttons: ['OK'],
          checkboxLabel: 'Do not show again',
          checkboxChecked: false
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.checkboxLabel).to.equal('Do not show again');
        expect(info.checkboxChecked).to.be.false();

        dialogHelper.clickMessageBoxButton(handle, 0);
        const result = await p;
        expect(result.checkboxChecked).to.be.false();
      });

      it('returns checkboxChecked as true when checkbox is initially checked', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Pre-checked checkbox',
          buttons: ['OK'],
          checkboxLabel: 'Remember my choice',
          checkboxChecked: true
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.checkboxLabel).to.equal('Remember my choice');
        expect(info.checkboxChecked).to.be.true();

        dialogHelper.clickMessageBoxButton(handle, 0);
        const result = await p;
        expect(result.checkboxChecked).to.be.true();
      });

      it('can toggle checkbox and returns updated state', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Toggle test',
          buttons: ['OK'],
          checkboxLabel: 'Toggle me',
          checkboxChecked: false
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();

        // Verify initially unchecked.
        let info = dialogHelper.getDialogInfo(handle);
        expect(info.checkboxChecked).to.be.false();

        // Click the checkbox to check it.
        dialogHelper.clickCheckbox(handle);
        info = dialogHelper.getDialogInfo(handle);
        expect(info.checkboxChecked).to.be.true();

        dialogHelper.clickMessageBoxButton(handle, 0);
        const result = await p;
        expect(result.checkboxChecked).to.be.true();
      });

      it('strips access keys on macOS with normalizeAccessKeys', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Access key test',
          buttons: ['&Save', '&Cancel'],
          normalizeAccessKeys: true
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        // On macOS, ampersands are stripped by normalizeAccessKeys.
        const buttons = JSON.parse(info.buttons);
        expect(buttons).to.include('Save');
        expect(buttons).to.include('Cancel');
        expect(buttons).not.to.include('&Save');
        expect(buttons).not.to.include('&Cancel');

        dialogHelper.clickMessageBoxButton(handle, 0);
        await p;
      });

      it('respects defaultId by making it the default button', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Default button test',
          buttons: ['One', 'Two', 'Three'],
          defaultId: 2
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();

        const info = dialogHelper.getDialogInfo(handle);
        const buttons = JSON.parse(info.buttons);
        expect(buttons).to.deep.equal(['One', 'Two', 'Three']);

        dialogHelper.clickMessageBoxButton(handle, 2);
        const result = await p;
        expect(result.response).to.equal(2);
      });

      it('respects cancelId and returns it when cancelled via signal', async () => {
        const controller = new AbortController();
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'Cancel ID test',
          buttons: ['OK', 'Dismiss', 'Abort'],
          cancelId: 2,
          signal: controller.signal
        });

        await waitForSheet(w);
        controller.abort();

        const result = await p;
        expect(result.response).to.equal(2);
      });

      it('works with all message box types', async () => {
        const types: Array<'none' | 'info' | 'warning' | 'error' | 'question'> =
          ['none', 'info', 'warning', 'error', 'question'];

        for (const type of types) {
          const w = new BrowserWindow({ show: false });
          const p = dialog.showMessageBox(w, {
            message: `Type: ${type}`,
            type,
            buttons: ['OK']
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);

          expect(info.type).to.equal('message-box');
          expect(info.message).to.equal(`Type: ${type}`);

          dialogHelper.clickMessageBoxButton(handle, 0);
          await p;
          w.destroy();
          // Allow the event loop to settle between iterations to avoid
          // Chromium DCHECK failures from rapid window lifecycle churn.
          await setTimeout(100);
        }
      });
    });

    describe('showOpenDialog', () => {
      it('can cancel an open dialog', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          title: 'Test Open',
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.type).to.equal('open-dialog');

        dialogHelper.cancelFileDialog(handle);

        const result = await p;
        expect(result.canceled).to.be.true();
        expect(result.filePaths).to.have.lengthOf(0);
      });

      it('sets a custom button label', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          buttonLabel: 'Select This',
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.prompt).to.equal('Select This');

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('sets a message on the dialog', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          message: 'Choose a file to import',
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.panelMessage).to.equal('Choose a file to import');

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('defaults to openFile with canChooseFiles enabled', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.canChooseFiles).to.be.true();
        expect(info.canChooseDirectories).to.be.false();
        expect(info.allowsMultipleSelection).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('enables directory selection with openDirectory', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.canChooseDirectories).to.be.true();
        // openFile is not set, so canChooseFiles should be false
        expect(info.canChooseFiles).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('enables both file and directory selection together', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile', 'openDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.canChooseFiles).to.be.true();
        expect(info.canChooseDirectories).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('enables multiple selection with multiSelections', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile', 'multiSelections']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.allowsMultipleSelection).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('shows hidden files with showHiddenFiles', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile', 'showHiddenFiles']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.showsHiddenFiles).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('does not show hidden files by default', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.showsHiddenFiles).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('disables alias resolution with noResolveAliases', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile', 'noResolveAliases']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.resolvesAliases).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('resolves aliases by default', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.resolvesAliases).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('treats packages as directories with treatPackageAsDirectory', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile', 'treatPackageAsDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.treatsPackagesAsDirectories).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('enables directory creation with createDirectory', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          properties: ['openFile', 'createDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.canCreateDirectories).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('sets the default path directory', async () => {
        const defaultDir = path.join(__dirname, 'fixtures');
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          defaultPath: defaultDir,
          properties: ['openFile']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.directory).to.equal(defaultDir);

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('applies multiple properties simultaneously', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          title: 'Multi-Property Test',
          buttonLabel: 'Pick',
          message: 'Select items',
          properties: [
            'openFile',
            'openDirectory',
            'multiSelections',
            'showHiddenFiles',
            'createDirectory',
            'treatPackageAsDirectory',
            'noResolveAliases'
          ]
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.type).to.equal('open-dialog');
        expect(info.prompt).to.equal('Pick');
        expect(info.panelMessage).to.equal('Select items');
        expect(info.canChooseFiles).to.be.true();
        expect(info.canChooseDirectories).to.be.true();
        expect(info.allowsMultipleSelection).to.be.true();
        expect(info.showsHiddenFiles).to.be.true();
        expect(info.canCreateDirectories).to.be.true();
        expect(info.treatsPackagesAsDirectories).to.be.true();
        expect(info.resolvesAliases).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('can accept an open dialog and return a file path', async () => {
        const targetDir = path.join(__dirname, 'fixtures');
        const w = new BrowserWindow({ show: false });
        const p = dialog.showOpenDialog(w, {
          defaultPath: targetDir,
          properties: ['openDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();

        dialogHelper.acceptFileDialog(handle);

        const result = await p;
        expect(result.canceled).to.be.false();
        expect(result.filePaths).to.have.lengthOf(1);
        expect(result.filePaths[0]).to.equal(targetDir);
      });
    });

    describe('showSaveDialog', () => {
      it('can cancel a save dialog', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          title: 'Test Save'
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.type).to.equal('save-dialog');

        dialogHelper.cancelFileDialog(handle);

        const result = await p;
        expect(result.canceled).to.be.true();
        expect(result.filePath).to.equal('');
      });

      it('can accept a save dialog with a filename', async () => {
        const defaultDir = path.join(__dirname, 'fixtures');
        const filename = 'test-save-output.txt';
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          title: 'Test Save',
          defaultPath: path.join(defaultDir, filename)
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();

        dialogHelper.acceptFileDialog(handle);

        const result = await p;
        expect(result.canceled).to.be.false();
        expect(result.filePath).to.equal(path.join(defaultDir, filename));
      });

      it('sets a custom button label', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          buttonLabel: 'Export'
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.prompt).to.equal('Export');

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('sets a message on the dialog', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          message: 'Choose where to save'
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.panelMessage).to.equal('Choose where to save');

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('sets a custom name field label', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          nameFieldLabel: 'Export As:'
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.nameFieldLabel).to.equal('Export As:');

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('sets the default filename from defaultPath', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          defaultPath: path.join(__dirname, 'fixtures', 'my-document.txt')
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.nameFieldValue).to.equal('my-document.txt');

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('sets the default directory from defaultPath', async () => {
        const defaultDir = path.join(__dirname, 'fixtures');
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          defaultPath: path.join(defaultDir, 'some-file.txt')
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.directory).to.equal(defaultDir);

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('hides the tag field when showsTagField is false', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          showsTagField: false
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.showsTagField).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('shows the tag field by default', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {});

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.showsTagField).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('enables directory creation with createDirectory', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          properties: ['createDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.canCreateDirectories).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('shows hidden files with showHiddenFiles', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          properties: ['showHiddenFiles']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.showsHiddenFiles).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('does not show hidden files by default', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {});

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.showsHiddenFiles).to.be.false();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('treats packages as directories with treatPackageAsDirectory', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          properties: ['treatPackageAsDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);
        expect(info.treatsPackagesAsDirectories).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });

      it('applies multiple options simultaneously', async () => {
        const defaultDir = path.join(__dirname, 'fixtures');
        const w = new BrowserWindow({ show: false });
        const p = dialog.showSaveDialog(w, {
          buttonLabel: 'Save Now',
          message: 'Pick a location',
          nameFieldLabel: 'File Name:',
          defaultPath: path.join(defaultDir, 'output.txt'),
          showsTagField: false,
          properties: ['showHiddenFiles', 'createDirectory']
        });

        await waitForSheet(w);
        const handle = w.getNativeWindowHandle();
        const info = dialogHelper.getDialogInfo(handle);

        expect(info.type).to.equal('save-dialog');
        expect(info.prompt).to.equal('Save Now');
        expect(info.panelMessage).to.equal('Pick a location');
        expect(info.nameFieldLabel).to.equal('File Name:');
        expect(info.nameFieldValue).to.equal('output.txt');
        expect(info.directory).to.equal(defaultDir);
        expect(info.showsTagField).to.be.false();
        expect(info.showsHiddenFiles).to.be.true();
        expect(info.canCreateDirectories).to.be.true();

        dialogHelper.cancelFileDialog(handle);
        await p;
      });
    });
  });
});
