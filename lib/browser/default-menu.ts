import { app, Menu } from 'electron/main';
import { shell } from 'electron/common';

const v8Util = process._linkedBinding('electron_common_v8_util');

const isMac = process.platform === 'darwin';

export const setDefaultApplicationMenu = () => {
  if (v8Util.getHiddenValue<boolean>(global, 'applicationMenuSet')) return;

  const helpMenu: Electron.MenuItemConstructorOptions = {
    role: 'help',
    submenu: app.isPackaged ? [] : [
      {
        label: 'Learn More',
        click: async () => {
          await shell.openExternal('https://electronjs.org');
        }
      },
      {
        label: 'Documentation',
        click: async () => {
          const version = process.versions.electron;
          await shell.openExternal(`https://github.com/electron/electron/tree/v${version}/docs#readme`);
        }
      },
      {
        label: 'Community Discussions',
        click: async () => {
          await shell.openExternal('https://discord.com/invite/APGC3k5yaH');
        }
      },
      {
        label: 'Search Issues',
        click: async () => {
          await shell.openExternal('https://github.com/electron/electron/issues');
        }
      }
    ]
  };

  const macAppMenu: Electron.MenuItemConstructorOptions = { role: 'appMenu' };
  const template: Electron.MenuItemConstructorOptions[] = [
    ...(isMac ? [macAppMenu] : []),
    { role: 'fileMenu' },
    { role: 'editMenu' },
    { role: 'viewMenu' },
    { role: 'windowMenu' },
    helpMenu
  ];

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
};
