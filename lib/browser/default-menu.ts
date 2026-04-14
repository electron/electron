import { Menu } from 'electron/main';

const isMac = process.platform === 'darwin';

let applicationMenuWasSet = false;

export const setApplicationMenuWasSet = () => {
  applicationMenuWasSet = true;
};

export const setDefaultApplicationMenu = () => {
  if (applicationMenuWasSet) return;

  const macAppMenu: Electron.MenuItemConstructorOptions = { role: 'appMenu' };
  const template: Electron.MenuItemConstructorOptions[] = [
    ...(isMac ? [macAppMenu] : []),
    { role: 'fileMenu' },
    { role: 'editMenu' },
    { role: 'viewMenu' },
    { role: 'windowMenu' }
  ];

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
};
