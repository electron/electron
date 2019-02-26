import { shell, Menu } from 'electron'

const v8Util = process.atomBinding('v8_util')

const isMac = process.platform === 'darwin'

export const setDefaultApplicationMenu = () => {
  if (v8Util.getHiddenValue<boolean>(global, 'applicationMenuSet')) return

  const helpMenu: Electron.MenuItemConstructorOptions = {
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click () {
          shell.openExternalSync('https://electronjs.org')
        }
      },
      {
        label: 'Documentation',
        click () {
          shell.openExternalSync(
            `https://github.com/electron/electron/tree/v${process.versions.electron}/docs#readme`
          )
        }
      },
      {
        label: 'Community Discussions',
        click () {
          shell.openExternalSync('https://discuss.atom.io/c/electron')
        }
      },
      {
        label: 'Search Issues',
        click () {
          shell.openExternalSync('https://github.com/electron/electron/issues')
        }
      }
    ]
  }

  const macAppMenu: Electron.MenuItemConstructorOptions = { role: 'appMenu' }
  const template: Electron.MenuItemConstructorOptions[] = [
    ...(isMac ? [macAppMenu] : []),
    { role: 'fileMenu' },
    { role: 'editMenu' },
    { role: 'viewMenu' },
    { role: 'windowMenu' },
    helpMenu
  ]

  const menu = Menu.buildFromTemplate(template)
  Menu.setApplicationMenu(menu)
}
