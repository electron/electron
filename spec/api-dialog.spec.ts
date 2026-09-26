import { app, dialog, BaseWindow, BrowserWindow } from 'electron/main';

import * as dbus from 'dbus-native';
import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { createRequire } from 'node:module';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import { promisify } from 'node:util';

import { ifdescribe, ifit, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

describe('dialog module', () => {
  describe('showOpenDialog', () => {
    afterEach(closeAllWindows);
    ifit(process.platform !== 'win32')('should not throw for valid cases', () => {
      expect(() => {
        dialog.showOpenDialog({ title: 'i am title' });
      }).not.toThrow();

      expect(() => {
        const w = new BrowserWindow();
        dialog.showOpenDialog(w, { title: 'i am title' });
      }).not.toThrow();

      expect(() => {
        const w = new BaseWindow();
        dialog.showOpenDialog(w, { title: 'i am title' });
      }).not.toThrow();
    });

    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showOpenDialog({ properties: false as any });
      }).toThrow(/Properties must be an array/);

      expect(() => {
        dialog.showOpenDialog({ title: 300 as any });
      }).toThrow(/Title must be a string/);

      expect(() => {
        dialog.showOpenDialog({ buttonLabel: [] as any });
      }).toThrow(/Button label must be a string/);

      expect(() => {
        dialog.showOpenDialog({ defaultPath: {} as any });
      }).toThrow(/Default path must be a string/);

      expect(() => {
        dialog.showOpenDialog({ message: {} as any });
      }).toThrow(/Message must be a string/);
    });
  });

  describe('showSaveDialog', () => {
    afterEach(closeAllWindows);
    ifit(process.platform !== 'win32')('should not throw for valid cases', () => {
      expect(() => {
        dialog.showSaveDialog({ title: 'i am title' });
      }).not.toThrow();

      expect(() => {
        const w = new BrowserWindow();
        dialog.showSaveDialog(w, { title: 'i am title' });
      }).not.toThrow();

      expect(() => {
        const w = new BaseWindow();
        dialog.showSaveDialog(w, { title: 'i am title' });
      }).not.toThrow();
    });

    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showSaveDialog({ title: 300 as any });
      }).toThrow(/Title must be a string/);

      expect(() => {
        dialog.showSaveDialog({ buttonLabel: [] as any });
      }).toThrow(/Button label must be a string/);

      expect(() => {
        dialog.showSaveDialog({ defaultPath: {} as any });
      }).toThrow(/Default path must be a string/);

      expect(() => {
        dialog.showSaveDialog({ message: {} as any });
      }).toThrow(/Message must be a string/);

      expect(() => {
        dialog.showSaveDialog({ nameFieldLabel: {} as any });
      }).toThrow(/Name field label must be a string/);
    });
  });

  describe('showMessageBox', () => {
    afterEach(closeAllWindows);

    // parentless message boxes are synchronous on macOS
    // dangling message boxes on windows cause a DCHECK: https://source.chromium.org/chromium/chromium/src/+/main:base/win/message_window.cc;drc=7faa4bf236a866d007dc5672c9ce42660e67a6a6;l=68
    ifit(process.platform !== 'darwin' && process.platform !== 'win32')(
      'should not throw for a parentless message box',
      () => {
        expect(() => {
          dialog.showMessageBox({ message: 'i am message' });
        }).not.toThrow();
      }
    );

    ifit(process.platform !== 'win32')('should not throw for valid cases', () => {
      expect(() => {
        const w = new BrowserWindow();
        dialog.showMessageBox(w, { message: 'i am message' });
      }).not.toThrow();

      expect(() => {
        const w = new BaseWindow();
        dialog.showMessageBox(w, { message: 'i am message' });
      }).not.toThrow();
    });

    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showMessageBox(undefined as any, { type: 'not-a-valid-type' as any, message: '' });
      }).toThrow(/Invalid message box type/);

      expect(() => {
        dialog.showMessageBox(null as any, { buttons: false as any, message: '' });
      }).toThrow(/Buttons must be an array/);

      expect(() => {
        dialog.showMessageBox({ title: 300 as any, message: '' });
      }).toThrow(/Title must be a string/);

      expect(() => {
        dialog.showMessageBox({ message: [] as any });
      }).toThrow(/Message must be a string/);

      expect(() => {
        dialog.showMessageBox({ detail: 3.14 as any, message: '' });
      }).toThrow(/Detail must be a string/);

      expect(() => {
        dialog.showMessageBox({ checkboxLabel: false as any, message: '' });
      }).toThrow(/checkboxLabel must be a string/);
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
      expect(result.response).toBe(0);
    });

    it('closes message box after a while', async () => {
      const controller = new AbortController();
      const signal = controller.signal;
      const w = new BrowserWindow();
      const p = dialog.showMessageBox(w, { signal, message: 'i am message' });
      await setTimeout(500);
      controller.abort();
      const result = await p;
      expect(result.response).toBe(0);
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
      expect(result.response).toBe(0);
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
      expect(result.response).toBe(1);
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
      expect(result.response).toBe(1);
    });
  });

  describe('showErrorBox', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        (dialog.showErrorBox as any)();
      }).toThrow(/Insufficient number of arguments/);

      expect(() => {
        dialog.showErrorBox(3 as any, 'four');
      }).toThrow(/Error processing argument at index 0/);

      expect(() => {
        dialog.showErrorBox('three', 4 as any);
      }).toThrow(/Error processing argument at index 1/);
    });
  });

  describe('showCertificateTrustDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        (dialog.showCertificateTrustDialog as any)();
      }).toThrow(/options must be an object/);

      expect(() => {
        dialog.showCertificateTrustDialog({} as any);
      }).toThrow(/certificate must be an object/);

      expect(() => {
        dialog.showCertificateTrustDialog({ certificate: {} as any, message: false as any });
      }).toThrow(/message must be a string/);
    });
  });

  ifdescribe(process.platform === 'darwin' && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)(
    'end-to-end dialog interaction (macOS)',
    () => {
      let dialogHelper: any;

      beforeAll(() => {
        dialogHelper = require('@electron-ci/dialog-helper');
      });

      afterEach(closeAllWindows);

      // Poll for a sheet to appear on the given window.
      async function waitForSheet(w: BrowserWindow): Promise<void> {
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

          expect(info.type).toBe('message-box');
          expect(info.message).toBe('Test message');

          const buttons = JSON.parse(info.buttons);
          expect(buttons).toContain('OK');
          expect(buttons).toContain('Cancel');

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

          expect(info.message).toBe('Main message');
          expect(info.detail).toBe('Extra detail text');

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
          expect(result.response).toBe(1);
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
          expect(result.response).toBe(2);
        });

        it('shows a single button when no buttons are specified', async () => {
          const w = new BrowserWindow({ show: false });
          const p = dialog.showMessageBox(w, {
            message: 'No buttons specified'
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);

          expect(info.type).toBe('message-box');
          // macOS adds a default "OK" button when none are specified.
          const buttons = JSON.parse(info.buttons);
          expect(buttons).toHaveLength(1);

          dialogHelper.clickMessageBoxButton(handle, 0);
          const result = await p;
          expect(result.response).toBe(0);
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

          expect(info.checkboxLabel).toBe('Do not show again');
          expect(info.checkboxChecked).toBe(false);

          dialogHelper.clickMessageBoxButton(handle, 0);
          const result = await p;
          expect(result.checkboxChecked).toBe(false);
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

          expect(info.checkboxLabel).toBe('Remember my choice');
          expect(info.checkboxChecked).toBe(true);

          dialogHelper.clickMessageBoxButton(handle, 0);
          const result = await p;
          expect(result.checkboxChecked).toBe(true);
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
          expect(info.checkboxChecked).toBe(false);

          // Click the checkbox to check it.
          dialogHelper.clickCheckbox(handle);
          info = dialogHelper.getDialogInfo(handle);
          expect(info.checkboxChecked).toBe(true);

          dialogHelper.clickMessageBoxButton(handle, 0);
          const result = await p;
          expect(result.checkboxChecked).toBe(true);
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
          expect(buttons).toContain('Save');
          expect(buttons).toContain('Cancel');
          expect(buttons).not.toContain('&Save');
          expect(buttons).not.toContain('&Cancel');

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
          expect(buttons).toEqual(['One', 'Two', 'Three']);

          dialogHelper.clickMessageBoxButton(handle, 2);
          const result = await p;
          expect(result.response).toBe(2);
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
          expect(result.response).toBe(2);
        });

        it('works with all message box types', async () => {
          const types: Array<'none' | 'info' | 'warning' | 'error' | 'question'> = [
            'none',
            'info',
            'warning',
            'error',
            'question'
          ];

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

            expect(info.type).toBe('message-box');
            expect(info.message).toBe(`Type: ${type}`);

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
          expect(info.type).toBe('open-dialog');

          dialogHelper.cancelFileDialog(handle);

          const result = await p;
          expect(result.canceled).toBe(true);
          expect(result.filePaths).toHaveLength(0);
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
          expect(info.prompt).toBe('Select This');

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
          expect(info.panelMessage).toBe('Choose a file to import');

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
          expect(info.canChooseFiles).toBe(true);
          expect(info.canChooseDirectories).toBe(false);
          expect(info.allowsMultipleSelection).toBe(false);

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
          expect(info.canChooseDirectories).toBe(true);
          // openFile is not set, so canChooseFiles should be false
          expect(info.canChooseFiles).toBe(false);

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
          expect(info.canChooseFiles).toBe(true);
          expect(info.canChooseDirectories).toBe(true);

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
          expect(info.allowsMultipleSelection).toBe(true);

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
          expect(info.showsHiddenFiles).toBe(true);

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
          expect(info.showsHiddenFiles).toBe(false);

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
          expect(info.resolvesAliases).toBe(false);

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
          expect(info.resolvesAliases).toBe(true);

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
          expect(info.treatsPackagesAsDirectories).toBe(true);

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
          expect(info.canCreateDirectories).toBe(true);

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('sets the default path directory', async () => {
          const defaultDir = path.join(import.meta.dirname, 'fixtures');
          const w = new BrowserWindow({ show: false });
          const p = dialog.showOpenDialog(w, {
            defaultPath: defaultDir,
            properties: ['openFile']
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);
          expect(info.directory).toBe(defaultDir);

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

          expect(info.type).toBe('open-dialog');
          expect(info.prompt).toBe('Pick');
          expect(info.panelMessage).toBe('Select items');
          expect(info.canChooseFiles).toBe(true);
          expect(info.canChooseDirectories).toBe(true);
          expect(info.allowsMultipleSelection).toBe(true);
          expect(info.showsHiddenFiles).toBe(true);
          expect(info.canCreateDirectories).toBe(true);
          expect(info.treatsPackagesAsDirectories).toBe(true);
          expect(info.resolvesAliases).toBe(false);

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('can accept an open dialog and return a file path', async () => {
          const targetDir = path.join(import.meta.dirname, 'fixtures');
          const w = new BrowserWindow({ show: false });
          const p = dialog.showOpenDialog(w, {
            defaultPath: targetDir,
            properties: ['openDirectory']
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();

          dialogHelper.acceptFileDialog(handle);

          const result = await p;
          expect(result.canceled).toBe(false);
          expect(result.filePaths).toHaveLength(1);
          expect(result.filePaths[0]).toBe(targetDir);
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
          expect(info.type).toBe('save-dialog');

          dialogHelper.cancelFileDialog(handle);

          const result = await p;
          expect(result.canceled).toBe(true);
          expect(result.filePath).toBe('');
        });

        it('can accept a save dialog with a filename', async () => {
          const defaultDir = path.join(import.meta.dirname, 'fixtures');
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
          expect(result.canceled).toBe(false);
          expect(result.filePath).toBe(path.join(defaultDir, filename));
        });

        it('sets a custom button label', async () => {
          const w = new BrowserWindow({ show: false });
          const p = dialog.showSaveDialog(w, {
            buttonLabel: 'Export'
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);
          expect(info.prompt).toBe('Export');

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
          expect(info.panelMessage).toBe('Choose where to save');

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
          expect(info.nameFieldLabel).toBe('Export As:');

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('sets the default filename from defaultPath', async () => {
          const w = new BrowserWindow({ show: false });
          const p = dialog.showSaveDialog(w, {
            defaultPath: path.join(import.meta.dirname, 'fixtures', 'my-document.txt')
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);
          expect(info.nameFieldValue).toBe('my-document.txt');

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('sets the default directory from defaultPath', async () => {
          const defaultDir = path.join(import.meta.dirname, 'fixtures');
          const w = new BrowserWindow({ show: false });
          const p = dialog.showSaveDialog(w, {
            defaultPath: path.join(defaultDir, 'some-file.txt')
          });

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);
          expect(info.directory).toBe(defaultDir);

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
          expect(info.showsTagField).toBe(false);

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('shows the tag field by default', async () => {
          const w = new BrowserWindow({ show: false });
          const p = dialog.showSaveDialog(w, {});

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);
          expect(info.showsTagField).toBe(true);

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
          expect(info.canCreateDirectories).toBe(true);

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
          expect(info.showsHiddenFiles).toBe(true);

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('does not show hidden files by default', async () => {
          const w = new BrowserWindow({ show: false });
          const p = dialog.showSaveDialog(w, {});

          await waitForSheet(w);
          const handle = w.getNativeWindowHandle();
          const info = dialogHelper.getDialogInfo(handle);
          expect(info.showsHiddenFiles).toBe(false);

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
          expect(info.treatsPackagesAsDirectories).toBe(true);

          dialogHelper.cancelFileDialog(handle);
          await p;
        });

        it('applies multiple options simultaneously', async () => {
          const defaultDir = path.join(import.meta.dirname, 'fixtures');
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

          expect(info.type).toBe('save-dialog');
          expect(info.prompt).toBe('Save Now');
          expect(info.panelMessage).toBe('Pick a location');
          expect(info.nameFieldLabel).toBe('File Name:');
          expect(info.nameFieldValue).toBe('output.txt');
          expect(info.directory).toBe(defaultDir);
          expect(info.showsTagField).toBe(false);
          expect(info.showsHiddenFiles).toBe(true);
          expect(info.canCreateDirectories).toBe(true);

          dialogHelper.cancelFileDialog(handle);
          await p;
        });
      });
    }
  );

  // Asserts on the D-Bus requests received by the mock xdg-desktop-portal
  // FileChooser that script/dbus_mock.py hosts on the fake session bus. The
  // mock records each request and auto-cancels the dialog.
  ifdescribe(
    process.platform === 'linux' &&
      process.arch !== 'ia32' &&
      !process.arch.startsWith('arm') &&
      !!process.env.DBUS_SESSION_BUS_ADDRESS
  )('file dialogs (Linux portal)', () => {
    let bus: any;
    let getCalls: () => Promise<any[]>;
    let clearCalls: () => Promise<void>;

    beforeAll(async () => {
      bus = dbus.sessionBus();
      const service = bus.getService('org.freedesktop.portal.Desktop');
      const getInterface = promisify(service.getInterface.bind(service));
      const mock: any = await getInterface('/org/freedesktop/portal/desktop', 'org.freedesktop.DBus.Mock');
      getCalls = promisify(mock.GetCalls.bind(mock));
      clearCalls = promisify(mock.ClearCalls.bind(mock));
    });

    afterAll(() => {
      bus?.connection.end();
    });

    beforeEach(async () => {
      await clearCalls();
    });

    // A call is [timestamp, methodName, [parent_window, title, options]];
    // args and dict values are [signature, [value]] variant pairs.
    function getLoggedOptions(call: any) {
      const options: Record<string, any> = {};
      for (const entry of call[2][2][1][0]) {
        options[entry[0]] = entry[1][1][0];
      }
      return options;
    }

    // current_folder is an 'ay' (null-terminated byte array).
    function getCurrentFolder(options: Record<string, any>) {
      const bytes = Buffer.from(options.current_folder);
      const nul = bytes.indexOf(0);
      return bytes.toString('utf8', 0, nul === -1 ? bytes.length : nul);
    }

    async function getSingleCall(method: string) {
      const calls = (await getCalls()).filter((call) => call[1] === method);
      expect(calls).toHaveLength(1);
      return calls[0];
    }

    describe('showSaveDialog', () => {
      it('sends the defaultPath as current_folder and current_name', async () => {
        const defaultDir = path.join(import.meta.dirname, 'fixtures');
        const { canceled } = await dialog.showSaveDialog({
          defaultPath: path.join(defaultDir, 'save-target.txt')
        });
        expect(canceled).toBe(true);

        const options = getLoggedOptions(await getSingleCall('SaveFile'));
        expect(options.current_name).toBe('save-target.txt');
        expect(getCurrentFolder(options)).toBe(defaultDir);
      });

      // https://github.com/electron/electron/issues/52051
      it('sends an absolute current_folder for a directory-less defaultPath', async () => {
        const { canceled } = await dialog.showSaveDialog({
          defaultPath: 'test.jpeg'
        });
        expect(canceled).toBe(true);

        const options = getLoggedOptions(await getSingleCall('SaveFile'));
        expect(options.current_name).toBe('test.jpeg');
        const currentFolder = getCurrentFolder(options);
        expect(currentFolder).toSatisfy(path.isAbsolute);
        expect([app.getPath('downloads'), app.getPath('home')]).toContain(currentFolder);
      });
    });

    describe('showOpenDialog', () => {
      it('sends an absolute current_folder for a directory-less defaultPath', async () => {
        const { canceled } = await dialog.showOpenDialog({
          defaultPath: 'test.jpeg',
          properties: ['openFile']
        });
        expect(canceled).toBe(true);

        const options = getLoggedOptions(await getSingleCall('OpenFile'));
        const currentFolder = getCurrentFolder(options);
        expect(currentFolder).toSatisfy(path.isAbsolute);
        expect([app.getPath('downloads'), app.getPath('home')]).toContain(currentFolder);
      });
    });
  });

  ifdescribe(process.platform === 'win32' && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)(
    'external message injection (Windows)',
    () => {
      let dialogHelper: any;

      beforeAll(() => {
        dialogHelper = require('@electron-ci/dialog-helper');
      });

      afterEach(closeAllWindows);

      // Poll for the native task dialog to appear.
      async function waitForDialog(w: BrowserWindow): Promise<Buffer> {
        const handle = w.getNativeWindowHandle();
        for (let i = 0; i < 50; i++) {
          if (dialogHelper.getDialogInfo(handle).type === 'message-box') return handle;
          await setTimeout(100);
        }
        throw new Error('Timed out waiting for the message box to appear');
      }

      it('ignores button clicks for IDs it never created and keeps the dialog open', async () => {
        const w = new BrowserWindow({ show: false });
        const p = dialog.showMessageBox(w, {
          message: 'External injection test',
          buttons: ['Button A', 'Button B']
        });

        let resolvedEarly = false;
        p.then(
          () => {
            resolvedEarly = true;
          },
          () => {
            resolvedEarly = true;
          }
        );

        const handle = await waitForDialog(w);
        dialogHelper.clickMessageBoxButton(handle, 420);
        await setTimeout(500);
        expect(resolvedEarly, 'dialog closed on an unknown button ID').toBe(false);
        expect(dialogHelper.getDialogInfo(handle).type).toBe('message-box');

        // A real button still closes the dialog and returns its index. The
        // dialog runs on a separate STA thread, so rather than a single click
        // with a fixed deadline, re-inject the click until the dialog
        // acknowledges it by closing. If this times out, check that kIDStart
        // in dialog_helper_win.cc is in sync with message_box_win.cc.
        await waitUntil(
          () => {
            dialogHelper.clickMessageBoxButton(handle, 1);
            return dialogHelper.getDialogInfo(handle).type === 'none';
          },
          { rate: 100 }
        );
        const result = await p;
        expect(result.response).toBe(1);
      });
    }
  );
});
