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
      const roleList = [
        'close',
        'copy',
        'cut',
        'forcereload',
        'hide',
        'hideothers',
        'minimize',
        'paste',
        'pasteandmatchstyle',
        'quit',
        'redo',
        'reload',
        'resetzoom',
        'selectall',
        'toggledevtools',
        'togglefullscreen',
        'undo',
        'zoomin',
        'zoomout'
      ]

      for (let role in roleList) {
        const item = new MenuItem({role})
        assert.equal(item.getDefaultRoleAccelerator(), undefined)
      }
    })

    it('returns the correct default label', () => {
      const roleList = {
        'close': process.platform === 'darwin' ? 'Close Window' : 'Close',
        'copy': 'Copy',
        'cut': 'Cut',
        'forcereload': 'Force Reload',
        'hide': 'Hide Electron Test',
        'hideothers': 'Hide Others',
        'minimize': 'Minimize',
        'paste': 'Paste',
        'pasteandmatchstyle': 'Paste and Match Style',
        'quit': (process.platform === 'darwin') ? `Quit ${app.getName()}` : (process.platform === 'win32') ? 'Exit' : 'Quit',
        'redo': 'Redo',
        'reload': 'Reload',
        'resetzoom': 'Actual Size',
        'selectall': 'Select All',
        'toggledevtools': 'Toggle Developer Tools',
        'togglefullscreen': 'Toggle Full Screen',
        'undo': 'Undo',
        'zoomin': 'Zoom In',
        'zoomout': 'Zoom Out'
      }

      for (let role in roleList) {
        const item = new MenuItem({role})
        assert.equal(item.label, roleList[role])
      }
    })

    it('returns the correct default accelerator', () => {
      const roleList = {
        'close': 'CommandOrControl+W',
        'copy': 'CommandOrControl+C',
        'cut': 'CommandOrControl+X',
        'forcereload': 'Shift+CmdOrCtrl+R',
        'hide': 'Command+H',
        'hideothers': 'Command+Alt+H',
        'minimize': 'CommandOrControl+M',
        'paste': 'CommandOrControl+V',
        'pasteandmatchstyle': 'Shift+CommandOrControl+V',
        'quit': process.platform === 'win32' ? null : 'CommandOrControl+Q',
        'redo': process.platform === 'win32' ? 'Control+Y' : 'Shift+CommandOrControl+Z',
        'reload': 'CmdOrCtrl+R',
        'resetzoom': 'CommandOrControl+0',
        'selectall': 'CommandOrControl+A',
        'toggledevtools': process.platform === 'darwin' ? 'Alt+Command+I' : 'Ctrl+Shift+I',
        'togglefullscreen': process.platform === 'darwin' ? 'Control+Command+F' : 'F11',
        'undo': 'CommandOrControl+Z',
        'zoomin': 'CommandOrControl+Plus',
        'zoomout': 'CommandOrControl+-'
      }

      for (let role in roleList) {
        const item = new MenuItem({role})
        assert.equal(item.getDefaultRoleAccelerator(), roleList[role])
      }
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
