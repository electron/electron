const assert = require('assert')

const {remote} = require('electron')
const {BrowserWindow, app, Menu, MenuItem} = remote
const roles = require('../lib/browser/api/menu-item-roles')
const {closeWindow} = require('./window-helpers')

describe('MenuItems', () => {
  describe('MenuItem.click', () => {
    it('should be called with the item object passed', (done) => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        click: (item) => {
          assert.equal(item.constructor.name, 'MenuItem')
          assert.equal(item.label, 'text')
          done()
        }
      }])
      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
    })
  })

  describe('MenuItem with checked/radio property', () => {
    it('clicking an checkbox item should flip the checked property', () => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        type: 'checkbox'
      }])

      assert.equal(menu.items[0].checked, false)
      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
      assert.equal(menu.items[0].checked, true)
    })

    it('clicking an radio item should always make checked property true', () => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        type: 'radio'
      }])

      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
      assert.equal(menu.items[0].checked, true)
      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
      assert.equal(menu.items[0].checked, true)
    })

    describe('MenuItem group properties', () => {
      let template = []

      const findRadioGroups = (template) => {
        let groups = []
        let cur = null
        for (let i = 0; i <= template.length; i++) {
          if (cur && ((i === template.length) || (template[i].type !== 'radio'))) {
            cur.end = i
            groups.push(cur)
            cur = null
          } else if (!cur && i < template.length && template[i].type === 'radio') {
            cur = { begin: i }
          }
        }
        return groups
      }

      // returns array of checked menuitems in [begin,end)
      const findChecked = (menuItems, begin, end) => {
        let checked = []
        for (let i = begin; i < end; i++) {
          if (menuItems[i].checked) checked.push(i)
        }
        return checked
      }

      beforeEach(() => {
        for (let i = 0; i <= 10; i++) {
          template.push({
            label: `${i}`,
            type: 'radio'
          })
        }

        template.push({type: 'separator'})

        for (let i = 12; i <= 20; i++) {
          template.push({
            label: `${i}`,
            type: 'radio'
          })
        }
      })

      it('at least have one item checked in each group', () => {
        const menu = Menu.buildFromTemplate(template)
        menu.delegate.menuWillShow(menu)

        const groups = findRadioGroups(template)

        groups.forEach(g => {
          assert.deepEqual(findChecked(menu.items, g.begin, g.end), [g.begin])
        })
      })

      it('should assign groupId automatically', () => {
        const menu = Menu.buildFromTemplate(template)

        let usedGroupIds = new Set()
        const groups = findRadioGroups(template)
        groups.forEach(g => {
          const groupId = menu.items[g.begin].groupId

          // groupId should be previously unused
          assert(!usedGroupIds.has(groupId))
          usedGroupIds.add(groupId)

          // everything in the group should have the same id
          for (let i = g.begin; i < g.end; ++i) {
            assert.equal(menu.items[i].groupId, groupId)
          }
        })
      })

      it("setting 'checked' should flip other items' 'checked' property", () => {
        const menu = Menu.buildFromTemplate(template)

        const groups = findRadioGroups(template)
        groups.forEach(g => {
          assert.deepEqual(findChecked(menu.items, g.begin, g.end), [])

          menu.items[g.begin].checked = true
          assert.deepEqual(findChecked(menu.items, g.begin, g.end), [g.begin])

          menu.items[g.end - 1].checked = true
          assert.deepEqual(findChecked(menu.items, g.begin, g.end), [g.end - 1])
        })
      })
    })
  })

  describe('MenuItem role execution', () => {
    it('does not try to execute roles without a valid role property', () => {
      let win = new BrowserWindow({show: false, width: 200, height: 200})
      let item = new MenuItem({role: 'asdfghjkl'})

      const canExecute = roles.execute(item.role, win, win.webContents)
      assert.equal(false, canExecute)

      closeWindow(win).then(() => { win = null })
    })

    it('executes roles with native role functions', () => {
      let win = new BrowserWindow({show: false, width: 200, height: 200})
      let item = new MenuItem({role: 'reload'})

      const canExecute = roles.execute(item.role, win, win.webContents)
      assert.equal(true, canExecute)

      closeWindow(win).then(() => { win = null })
    })

    it('execute roles with non-native role functions', () => {
      let win = new BrowserWindow({show: false, width: 200, height: 200})
      let item = new MenuItem({role: 'resetzoom'})

      const canExecute = roles.execute(item.role, win, win.webContents)
      assert.equal(true, canExecute)

      closeWindow(win).then(() => { win = null })
    })
  })

  describe('MenuItem command id', () => {
    it('cannot be overwritten', () => {
      const item = new MenuItem({label: 'item'})

      const commandId = item.commandId
      assert(commandId)
      item.commandId = `${commandId}-modified`
      assert.equal(item.commandId, commandId)
    })
  })

  describe('MenuItem with invalid type', () => {
    it('throws an exception', () => {
      assert.throws(() => {
        Menu.buildFromTemplate([{
          label: 'text',
          type: 'not-a-type'
        }])
      }, /Unknown menu item type: not-a-type/)
    })
  })

  describe('MenuItem with submenu type and missing submenu', () => {
    it('throws an exception', () => {
      assert.throws(() => {
        Menu.buildFromTemplate([{
          label: 'text',
          type: 'submenu'
        }])
      }, /Invalid submenu/)
    })
  })

  describe('MenuItem role', () => {
    it('returns undefined for items without default accelerator', () => {
      let item = new MenuItem({role: 'about'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'delete'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'front'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'help'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'services'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'recentdocuments'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'clearrecentdocuments'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'startspeaking'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'stopspeaking'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'unhide'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'window'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'zoom'})
      assert.equal(item.getDefaultRoleAccelerator(), undefined)
    })

    it('returns the correct default accelerator and label', () => {
      let item = new MenuItem({role: 'close'})
      assert.equal(item.label, process.platform === 'darwin' ? 'Close Window' : 'Close')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+W')

      item = new MenuItem({role: 'copy'})
      assert.equal(item.label, 'Copy')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+C')

      item = new MenuItem({role: 'cut'})
      assert.equal(item.label, 'Cut')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+X')

      item = new MenuItem({role: 'forcereload'})
      assert.equal(item.label, 'Force Reload')
      assert.equal(item.getDefaultRoleAccelerator(), 'Shift+CmdOrCtrl+R')

      item = new MenuItem({role: 'hide'})
      assert.equal(item.label, 'Hide Electron Test')
      assert.equal(item.getDefaultRoleAccelerator(), 'Command+H')

      item = new MenuItem({role: 'hideothers'})
      assert.equal(item.label, 'Hide Others')
      assert.equal(item.getDefaultRoleAccelerator(), 'Command+Alt+H')

      item = new MenuItem({role: 'minimize'})
      assert.equal(item.label, 'Minimize')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+M')

      item = new MenuItem({role: 'paste'})
      assert.equal(item.label, 'Paste')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+V')

      item = new MenuItem({role: 'pasteandmatchstyle'})
      assert.equal(item.label, 'Paste and Match Style')
      assert.equal(item.getDefaultRoleAccelerator(), 'Shift+CommandOrControl+V')

      let label
      item = new MenuItem({role: 'quit'})
      if (process.platform === 'darwin') {
        label = `Quit ${app.getName()}`
      } else if (process.platform === 'win32') {
        label = 'Exit'
      } else {
        label = 'Quit'
      }
      assert.equal(item.label, label)
      assert.equal(item.getDefaultRoleAccelerator(), process.platform === 'win32' ? null : 'CommandOrControl+Q')

      item = new MenuItem({role: 'redo'})
      assert.equal(item.label, 'Redo')
      assert.equal(item.getDefaultRoleAccelerator(), process.platform === 'win32' ? 'Control+Y' : 'Shift+CommandOrControl+Z')

      item = new MenuItem({role: 'reload'})
      assert.equal(item.label, 'Reload')
      assert.equal(item.getDefaultRoleAccelerator(), 'CmdOrCtrl+R')

      item = new MenuItem({role: 'resetzoom'})
      assert.equal(item.label, 'Actual Size')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+0')

      item = new MenuItem({role: 'selectall'})
      assert.equal(item.label, 'Select All')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+A')

      item = new MenuItem({role: 'toggledevtools'})
      assert.equal(item.label, 'Toggle Developer Tools')
      assert.equal(item.getDefaultRoleAccelerator(), process.platform === 'darwin' ? 'Alt+Command+I' : 'Ctrl+Shift+I')

      item = new MenuItem({role: 'togglefullscreen'})
      assert.equal(item.label, 'Toggle Full Screen')
      assert.equal(item.getDefaultRoleAccelerator(), process.platform === 'darwin' ? 'Control+Command+F' : 'F11')

      item = new MenuItem({role: 'undo'})
      assert.equal(item.label, 'Undo')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+Z')

      item = new MenuItem({role: 'zoomin'})
      assert.equal(item.label, 'Zoom In')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+Plus')

      item = new MenuItem({role: 'zoomout'})
      assert.equal(item.label, 'Zoom Out')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+-')
    })

    it('allows a custom accelerator and label to be set', () => {
      const item = new MenuItem({
        role: 'close',
        label: 'Custom Close!',
        accelerator: 'D'
      })

      assert.equal(item.label, 'Custom Close!')
      assert.equal(item.accelerator, 'D')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+W')
    })
  })

  describe('MenuItem editMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({role: 'editMenu'})

      assert.equal(item.label, 'Edit')
      assert.equal(item.submenu.items[0].role, 'undo')
      assert.equal(item.submenu.items[1].role, 'redo')
      assert.equal(item.submenu.items[2].type, 'separator')
      assert.equal(item.submenu.items[3].role, 'cut')
      assert.equal(item.submenu.items[4].role, 'copy')
      assert.equal(item.submenu.items[5].role, 'paste')

      if (process.platform === 'darwin') {
        assert.equal(item.submenu.items[6].role, 'pasteandmatchstyle')
        assert.equal(item.submenu.items[7].role, 'delete')
        assert.equal(item.submenu.items[8].role, 'selectall')
      }

      if (process.platform === 'win32') {
        assert.equal(item.submenu.items[6].role, 'delete')
        assert.equal(item.submenu.items[7].type, 'separator')
        assert.equal(item.submenu.items[8].role, 'selectall')
      }
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'editMenu',
        submenu: [{
          role: 'close'
        }]
      })
      assert.equal(item.label, 'Edit')
      assert.equal(item.submenu.items[0].role, 'close')
    })
  })

  describe('MenuItem windowMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({role: 'windowMenu'})

      assert.equal(item.label, 'Window')
      assert.equal(item.submenu.items[0].role, 'minimize')
      assert.equal(item.submenu.items[1].role, 'close')

      if (process.platform === 'darwin') {
        assert.equal(item.submenu.items[2].type, 'separator')
        assert.equal(item.submenu.items[3].role, 'front')
      }
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'windowMenu',
        submenu: [{role: 'copy'}]
      })

      assert.equal(item.label, 'Window')
      assert.equal(item.submenu.items[0].role, 'copy')
    })
  })

  describe('MenuItem with custom properties in constructor', () => {
    it('preserves the custom properties', () => {
      const template = [{
        label: 'menu 1',
        customProp: 'foo',
        submenu: []
      }]

      const menu = Menu.buildFromTemplate(template)
      menu.items[0].submenu.append(new MenuItem({
        label: 'item 1',
        customProp: 'bar',
        overrideProperty: 'oops not allowed'
      }))

      assert.equal(menu.items[0].customProp, 'foo')
      assert.equal(menu.items[0].submenu.items[0].label, 'item 1')
      assert.equal(menu.items[0].submenu.items[0].customProp, 'bar')
      assert.equal(typeof menu.items[0].submenu.items[0].overrideProperty, 'function')
    })
  })
})
