import { IPC_MESSAGES } from '@electron/internal//common/ipc-messages';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';

import { dialog, Menu } from 'electron/main';

import * as fs from 'fs';

const convertToMenuTemplate = function (items: ContextMenuItem[], handler: (id: number) => void) {
  return items.map(function (item) {
    const transformed: Electron.MenuItemConstructorOptions = item.type === 'subMenu'
      ? {
          type: 'submenu',
          label: item.label,
          enabled: item.enabled,
          submenu: convertToMenuTemplate(item.subItems, handler)
        }
      : item.type === 'separator'
        ? {
            type: 'separator'
          }
        : item.type === 'checkbox'
          ? {
              type: 'checkbox',
              label: item.label,
              enabled: item.enabled,
              checked: item.checked
            }
          : {
              type: 'normal',
              label: item.label,
              enabled: item.enabled
            };

    if (item.id != null) {
      transformed.click = () => handler(item.id);
    }

    return transformed;
  });
};

const getEditMenuItems = function (): Electron.MenuItemConstructorOptions[] {
  return [
    { role: 'undo' },
    { role: 'redo' },
    { type: 'separator' },
    { role: 'cut' },
    { role: 'copy' },
    { role: 'paste' },
    { role: 'pasteAndMatchStyle' },
    { role: 'delete' },
    { role: 'selectAll' }
  ];
};

const isChromeDevTools = function (pageURL: string) {
  const { protocol } = new URL(pageURL);
  return protocol === 'devtools:';
};

const assertChromeDevTools = function (contents: Electron.WebContents, api: string) {
  const pageURL = contents.getURL();
  if (!isChromeDevTools(pageURL)) {
    console.error(`Blocked ${pageURL} from calling ${api}`);
    throw new Error(`Blocked ${api}`);
  }
};

ipcMainInternal.handle(IPC_MESSAGES.INSPECTOR_CONTEXT_MENU, function (event, items: ContextMenuItem[], isEditMenu: boolean) {
  return new Promise<number | void>(resolve => {
    assertChromeDevTools(event.sender, 'window.InspectorFrontendHost.showContextMenuAtPoint()');

    const template = isEditMenu ? getEditMenuItems() : convertToMenuTemplate(items, resolve);
    const menu = Menu.buildFromTemplate(template);
    const window = event.sender.getOwnerBrowserWindow()!;

    menu.popup({ window, callback: () => resolve() });
  });
});

ipcMainInternal.handle(IPC_MESSAGES.INSPECTOR_SELECT_FILE, async function (event) {
  assertChromeDevTools(event.sender, 'window.UI.createFileSelectorElement()');

  const result = await dialog.showOpenDialog({});
  if (result.canceled) return [];

  const path = result.filePaths[0];
  const data = await fs.promises.readFile(path);

  return [path, data];
});

ipcMainUtils.handleSync(IPC_MESSAGES.INSPECTOR_CONFIRM, async function (event, message: string = '', title: string = '') {
  assertChromeDevTools(event.sender, 'window.confirm()');

  const options = {
    message: String(message),
    title: String(title),
    buttons: ['OK', 'Cancel'],
    cancelId: 1
  };
  const window = event.sender.getOwnerBrowserWindow()!;
  const { response } = await dialog.showMessageBox(window, options);
  return response === 0;
});
