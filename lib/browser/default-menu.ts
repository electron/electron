import { Menu } from 'electron/main';

const isMac = process.platform === 'darwin';

let applicationMenuWasSet = false;

export const setApplicationMenuWasSet = () => {
  applicationMenuWasSet = true;
};

export const setDefaultApplicationMenu = () => {
  if (applicationMenuWasSet) return;

  const helpMenu: Electron.MenuItemConstructorOptions = {
    role: 'help',
    submenu: []  
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
