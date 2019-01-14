const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { remote } = require('electron')
const { BrowserWindow, app, Menu, MenuItem } = remote
const roles = require('../lib/browser/api/menu-item-roles')
const { closeWindow } = require('./window-helpers')

const { expect, assert } = chai
chai.use(dirtyChai)

describe('MenuItems', () => {
  describe('MenuItem.click', () => {
    it('should be called with the item object passed', done => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        click: (item) => {
          expect(item.constructor.name).to.equal('MenuItem')
          expect(item.label).to.equal('text')
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

      expect(menu.items[0].checked).to.be.false()
      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
      expect(menu.items[0].checked).to.be.true()
    })

    it('clicking an radio item should always make checked property true', () => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        type: 'radio'
      }])

      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
      expect(menu.items[0].checked).to.be.true()
      menu.delegate.executeCommand(menu, {}, menu.items[0].commandId)
      expect(menu.items[0].checked).to.be.true()
    })

    describe('MenuItem group properties', () => {
      const template = []

      const findRadioGroups = (template) => {
        const groups = []
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
        const checked = []
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

        template.push({ type: 'separator' })

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
          expect(findChecked(menu.items, g.begin, g.end)).to.deep.equal([g.begin])
        })
      })

      it('should assign groupId automatically', () => {
        const menu = Menu.buildFromTemplate(template)

        const usedGroupIds = new Set()
        const groups = findRadioGroups(template)
        groups.forEach(g => {
          const groupId = menu.items[g.begin].groupId

          // groupId should be previously unused
          expect(usedGroupIds.has(groupId)).to.be.false()
          usedGroupIds.add(groupId)

          // everything in the group should have the same id
          for (let i = g.begin; i < g.end; ++i) {
            expect(menu.items[i].groupId).to.equal(groupId)
          }
        })
      })

      it("setting 'checked' should flip other items' 'checked' property", () => {
        const menu = Menu.buildFromTemplate(template)

        const groups = findRadioGroups(template)
        groups.forEach(g => {
          expect(findChecked(menu.items, g.begin, g.end)).to.deep.equal([])

          menu.items[g.begin].checked = true
          expect(findChecked(menu.items, g.begin, g.end)).to.deep.equal([g.begin])

          menu.items[g.end - 1].checked = true
          expect(findChecked(menu.items, g.begin, g.end)).to.deep.equal([g.end - 1])
        })
      })
    })
  })

  describe('MenuItem role execution', () => {
    it('does not try to execute roles without a valid role property', () => {
      let win = new BrowserWindow({ show: false, width: 200, height: 200 })
      const item = new MenuItem({ role: 'asdfghjkl' })

      const canExecute = roles.execute(item.role, win, win.webContents)
      expect(canExecute).to.be.false()

      closeWindow(win).then(() => { win = null })
    })

    it('executes roles with native role functions', () => {
      let win = new BrowserWindow({ show: false, width: 200, height: 200 })
      const item = new MenuItem({ role: 'reload' })

      const canExecute = roles.execute(item.role, win, win.webContents)
      expect(canExecute).to.be.true()

      closeWindow(win).then(() => { win = null })
    })

    it('execute roles with non-native role functions', () => {
      let win = new BrowserWindow({ show: false, width: 200, height: 200 })
      const item = new MenuItem({ role: 'resetzoom' })

      const canExecute = roles.execute(item.role, win, win.webContents)
      expect(canExecute).to.be.true()

      closeWindow(win).then(() => { win = null })
    })
  })

  describe('MenuItem command id', () => {
    it('cannot be overwritten', () => {
      const item = new MenuItem({ label: 'item' })

      const commandId = item.commandId
      expect(commandId).to.not.be.undefined()
      item.commandId = `${commandId}-modified`
      expect(item.commandId).to.equal(commandId)
    })
  })

  describe('MenuItem with invalid type', () => {
    it('throws an exception', () => {
      expect(() => {
        Menu.buildFromTemplate([{
          label: 'text',
          type: 'not-a-type'
        }])
      }).to.throw(/Unknown menu item type: not-a-type/)
    })
  })

  describe('MenuItem with submenu type and missing submenu', () => {
    it('throws an exception', () => {
      expect(() => {
        Menu.buildFromTemplate([{
          label: 'text',
          type: 'submenu'
        }])
      }).to.throw(/Invalid submenu/)
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

      for (const role in roleList) {
        const item = new MenuItem({ role })
        expect(item.getDefaultRoleAccelerator()).to.be.undefined()
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

      for (const role in roleList) {
        const item = new MenuItem({ role })
        expect(item.label).to.equal(roleList[role])
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

      for (const role in roleList) {
        const item = new MenuItem({ role })
        expect(item.getDefaultRoleAccelerator()).to.equal(roleList[role])
      }
    })

    it('allows a custom accelerator and label to be set', () => {
      const item = new MenuItem({
        role: 'close',
        label: 'Custom Close!',
        accelerator: 'D'
      })

      expect(item.label).to.equal('Custom Close!')
      expect(item.accelerator).to.equal('D')
      expect(item.getDefaultRoleAccelerator()).to.equal('CommandOrControl+W')
    })
  })

  describe('MenuItem appMenu', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'appMenu' })

      expect(item.label).to.equal(app.getName())
      expect(item.submenu.items[0].role).to.equal('about')
      expect(item.submenu.items[1].type).to.equal('separator')
      expect(item.submenu.items[2].role).to.equal('services')
      expect(item.submenu.items[3].type).to.equal('separator')
      expect(item.submenu.items[4].role).to.equal('hide')
      expect(item.submenu.items[5].role).to.equal('hideothers')
      expect(item.submenu.items[6].role).to.equal('unhide')
      expect(item.submenu.items[7].type).to.equal('separator')
      expect(item.submenu.items[8].role).to.equal('quit')
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'appMenu',
        submenu: [{
          role: 'close'
        }]
      })
      expect(item.label).to.equal(app.getName())
      expect(item.submenu.items[0].role).to.equal('close')
    })
  })

  describe('MenuItem fileMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'fileMenu' })

      expect(item.label).to.equal('File')
      if (process.platform === 'darwin') {
        expect(item.submenu.items[0].role).to.equal('close')
      } else {
        expect(item.submenu.items[0].role).to.equal('quit')
      }
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'fileMenu',
        submenu: [{
          role: 'about'
        }]
      })
      expect(item.label).to.equal('File')
      expect(item.submenu.items[0].role).to.equal('about')
    })
  })

  describe('MenuItem editMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'editMenu' })

      expect(item.label).to.equal('Edit')
      expect(item.submenu.items[0].role).to.equal('undo')
      expect(item.submenu.items[1].role).to.equal('redo')
      expect(item.submenu.items[2].type).to.equal('separator')
      expect(item.submenu.items[3].role).to.equal('cut')
      expect(item.submenu.items[4].role).to.equal('copy')
      expect(item.submenu.items[5].role).to.equal('paste')

      if (process.platform === 'darwin') {
        expect(item.submenu.items[6].role).to.equal('pasteandmatchstyle')
        expect(item.submenu.items[7].role).to.equal('delete')
        expect(item.submenu.items[8].role).to.equal('selectall')
        expect(item.submenu.items[9].type).to.equal('separator')
        expect(item.submenu.items[10].label).to.equal('Speech')
        expect(item.submenu.items[10].submenu.items[0].role).to.equal('startspeaking')
        expect(item.submenu.items[10].submenu.items[1].role).to.equal('stopspeaking')
      } else {
        expect(item.submenu.items[6].role).to.equal('delete')
        expect(item.submenu.items[7].type).to.equal('separator')
        expect(item.submenu.items[8].role).to.equal('selectall')
      }
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'editMenu',
        submenu: [{
          role: 'close'
        }]
      })
      expect(item.label).to.equal('Edit')
      expect(item.submenu.items[0].role).to.equal('close')
    })
  })

  describe('MenuItem viewMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'viewMenu' })

      expect(item.label).to.equal('View')
      expect(item.submenu.items[0].role).to.equal('reload')
      expect(item.submenu.items[1].role).to.equal('forcereload')
      expect(item.submenu.items[2].role).to.equal('toggledevtools')
      expect(item.submenu.items[3].type).to.equal('separator')
      expect(item.submenu.items[4].role).to.equal('resetzoom')
      expect(item.submenu.items[5].role).to.equal('zoomin')
      expect(item.submenu.items[6].role).to.equal('zoomout')
      expect(item.submenu.items[7].type).to.equal('separator')
      expect(item.submenu.items[8].role).to.equal('togglefullscreen')
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'viewMenu',
        submenu: [{
          role: 'close'
        }]
      })
      expect(item.label).to.equal('View')
      expect(item.submenu.items[0].role).to.equal('close')
    })
  })

  describe('MenuItem windowMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'windowMenu' })

      expect(item.label).to.equal('Window')
      expect(item.submenu.items[0].role).to.equal('minimize')
      expect(item.submenu.items[1].role).to.equal('zoom')

      if (process.platform === 'darwin') {
        expect(item.submenu.items[2].type).to.equal('separator')
        expect(item.submenu.items[3].role).to.equal('front')
      } else {
        expect(item.submenu.items[2].role).to.equal('close')
      }
    })

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'windowMenu',
        submenu: [{ role: 'copy' }]
      })

      expect(item.label).to.equal('Window')
      expect(item.submenu.items[0].role).to.equal('copy')
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

      expect(menu.items[0].customProp).to.equal('foo')
      expect(menu.items[0].submenu.items[0].label).to.equal('item 1')
      expect(menu.items[0].submenu.items[0].customProp).to.equal('bar')
      expect(menu.items[0].submenu.items[0].overrideProperty).to.be.a('function')
    })
  })

  describe('MenuItem accelerators', () => {
    const isDarwin = () => {
      return (process.platform === 'darwin')
    }

    it('should display modifiers correctly for simple keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'CmdOrCtrl+A' },
        { label: 'text', accelerator: 'Shift+A' },
        { label: 'text', accelerator: 'Alt+A' }
      ])

      assert.strictEqual(menu.getAcceleratorTextAt(0),
        isDarwin() ? '⌘A' : 'Ctrl+A')
      assert.strictEqual(menu.getAcceleratorTextAt(1),
        isDarwin() ? '⇧A' : 'Shift+A')
      assert.strictEqual(menu.getAcceleratorTextAt(2),
        isDarwin() ? '⌥A' : 'Alt+A')
    })

    it('should display modifiers correctly for special keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'CmdOrCtrl+Tab' },
        { label: 'text', accelerator: 'Shift+Tab' },
        { label: 'text', accelerator: 'Alt+Tab' }
      ])

      assert.strictEqual(menu.getAcceleratorTextAt(0),
        isDarwin() ? '⌘⇥\u0000' : 'Ctrl+Tab')
      assert.strictEqual(menu.getAcceleratorTextAt(1),
        isDarwin() ? '⇧⇥\u0000' : 'Shift+Tab')
      assert.strictEqual(menu.getAcceleratorTextAt(2),
        isDarwin() ? '⌥⇥\u0000' : 'Alt+Tab')
    })

    it('should not display modifiers twice', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'Shift+Shift+A' },
        { label: 'text', accelerator: 'Shift+Shift+Tab' }
      ])

      assert.strictEqual(menu.getAcceleratorTextAt(0),
        isDarwin() ? '⇧A' : 'Shift+A')
      assert.strictEqual(menu.getAcceleratorTextAt(1),
        isDarwin() ? '⇧⇥\u0000' : 'Shift+Tab')
    })

    it('should display correctly for edge cases', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'Control+Shift+=' },
        { label: 'text', accelerator: 'Control+Plus' }
      ])

      assert.strictEqual(menu.getAcceleratorTextAt(0),
        isDarwin() ? '⌃⇧=' : 'Ctrl+Shift+=')
      assert.strictEqual(menu.getAcceleratorTextAt(1),
        isDarwin() ? '⌃⇧=' : 'Ctrl+Shift+=')
    })
  })
})
