const assert = require('assert')

const {ipcRenderer, remote} = require('electron')
const {BrowserWindow, Menu, MenuItem} = remote
const {closeWindow} = require('./window-helpers')

describe('Menu module', () => {
  describe('Menu.buildFromTemplate', () => {
    it('should be able to attach extra fields', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          extra: 'field'
        }
      ])
      assert.equal(menu.items[0].extra, 'field')
    })

    it('does not modify the specified template', () => {
      const template = ipcRenderer.sendSync('eval', "var template = [{label: 'text', submenu: [{label: 'sub'}]}];\nrequire('electron').Menu.buildFromTemplate(template);\ntemplate;")
      assert.deepStrictEqual(template, [
        {
          label: 'text',
          submenu: [
            {
              label: 'sub'
            }
          ]
        }
      ])
    })

    it('does not throw exceptions for undefined/null values', () => {
      assert.doesNotThrow(() => {
        Menu.buildFromTemplate([
          {
            label: 'text',
            accelerator: undefined
          },
          {
            label: 'text again',
            accelerator: null
          }
        ])
      })
    })

    describe('Menu.buildFromTemplate should reorder based on item position specifiers', () => {
      it('should position before existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            label: '2',
            id: '2'
          }, {
            label: '3',
            id: '3'
          }, {
            label: '1',
            id: '1',
            position: 'before=2'
          }
        ])
        assert.equal(menu.items[0].label, '1')
        assert.equal(menu.items[1].label, '2')
        assert.equal(menu.items[2].label, '3')
      })

      it('should position after existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            label: '1',
            id: '1'
          }, {
            label: '3',
            id: '3'
          }, {
            label: '2',
            id: '2',
            position: 'after=1'
          }
        ])
        assert.equal(menu.items[0].label, '1')
        assert.equal(menu.items[1].label, '2')
        assert.equal(menu.items[2].label, '3')
      })

      it('should position at endof existing separator groups', () => {
        const menu = Menu.buildFromTemplate([
          {
            type: 'separator',
            id: 'numbers'
          }, {
            type: 'separator',
            id: 'letters'
          }, {
            label: 'a',
            id: 'a',
            position: 'endof=letters'
          }, {
            label: '1',
            id: '1',
            position: 'endof=numbers'
          }, {
            label: 'b',
            id: 'b',
            position: 'endof=letters'
          }, {
            label: '2',
            id: '2',
            position: 'endof=numbers'
          }, {
            label: 'c',
            id: 'c',
            position: 'endof=letters'
          }, {
            label: '3',
            id: '3',
            position: 'endof=numbers'
          }
        ])
        assert.equal(menu.items[0].id, 'numbers')
        assert.equal(menu.items[1].label, '1')
        assert.equal(menu.items[2].label, '2')
        assert.equal(menu.items[3].label, '3')
        assert.equal(menu.items[4].id, 'letters')
        assert.equal(menu.items[5].label, 'a')
        assert.equal(menu.items[6].label, 'b')
        assert.equal(menu.items[7].label, 'c')
      })

      it('should create separator group if endof does not reference existing separator group', () => {
        const menu = Menu.buildFromTemplate([
          {
            label: 'a',
            id: 'a',
            position: 'endof=letters'
          }, {
            label: '1',
            id: '1',
            position: 'endof=numbers'
          }, {
            label: 'b',
            id: 'b',
            position: 'endof=letters'
          }, {
            label: '2',
            id: '2',
            position: 'endof=numbers'
          }, {
            label: 'c',
            id: 'c',
            position: 'endof=letters'
          }, {
            label: '3',
            id: '3',
            position: 'endof=numbers'
          }
        ])
        assert.equal(menu.items[0].id, 'letters')
        assert.equal(menu.items[1].label, 'a')
        assert.equal(menu.items[2].label, 'b')
        assert.equal(menu.items[3].label, 'c')
        assert.equal(menu.items[4].id, 'numbers')
        assert.equal(menu.items[5].label, '1')
        assert.equal(menu.items[6].label, '2')
        assert.equal(menu.items[7].label, '3')
      })

      it('should continue inserting items at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          {
            label: '4',
            id: '4'
          }, {
            label: '5',
            id: '5'
          }, {
            label: '1',
            id: '1',
            position: 'before=4'
          }, {
            label: '2',
            id: '2'
          }, {
            label: '3',
            id: '3'
          }
        ])
        assert.equal(menu.items[0].label, '1')
        assert.equal(menu.items[1].label, '2')
        assert.equal(menu.items[2].label, '3')
        assert.equal(menu.items[3].label, '4')
        assert.equal(menu.items[4].label, '5')
      })
    })
  })

  describe('Menu.getMenuItemById', () => {
    it('should return the item with the given id', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'View',
          submenu: [
            {
              label: 'Enter Fullscreen',
              accelerator: 'Control+Command+F',
              id: 'fullScreen'
            },
            {
              label: 'Exit Fullscreen',
              accelerator: 'Control+Command+F'
            }
          ]
        }
      ])
      const fsc = menu.getMenuItemById('fullScreen')
      assert.equal(menu.items[0].submenu.items[0], fsc)
    })
  })

  describe('Menu.insert', () => {
    it('should store item in @items by its index', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: '1'
        }, {
          label: '2'
        }, {
          label: '3'
        }
      ])

      const item = new MenuItem({ label: 'inserted' })

      menu.insert(1, item)
      assert.equal(menu.items[0].label, '1')
      assert.equal(menu.items[1].label, 'inserted')
      assert.equal(menu.items[2].label, '2')
      assert.equal(menu.items[3].label, '3')
    })
  })

  describe('Menu.append', () => {
    it('should add the item to the end of the menu', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: '1'
        }, {
          label: '2'
        }, {
          label: '3'
        }
      ])
      const item = new MenuItem({ label: 'inserted' })

      menu.append(item)
      assert.equal(menu.items[0].label, '1')
      assert.equal(menu.items[1].label, '2')
      assert.equal(menu.items[2].label, '3')
      assert.equal(menu.items[3].label, 'inserted')
    })
  })

  describe('Menu.popup', () => {
    let w = null
    let menu

    beforeEach(() => {
      w = new BrowserWindow({show: false, width: 200, height: 200})
      menu = Menu.buildFromTemplate([
        {
          label: '1'
        }, {
          label: '2'
        }, {
          label: '3'
        }
      ])
    })

    afterEach(() => {
      menu.closePopup()
      menu.closePopup(w)
      return closeWindow(w).then(() => { w = null })
    })

    it('returns immediately', () => {
      const { browserWindow, x, y } = menu.popup(w, {x: 100, y: 101})

      assert.equal(browserWindow, w)
      assert.equal(x, 100)
      assert.equal(y, 101)
    })

    it('works without a given BrowserWindow and options', () => {
      const { browserWindow, x, y } = menu.popup({x: 100, y: 101})

      assert.equal(browserWindow.constructor.name, 'BrowserWindow')
      assert.equal(x, 100)
      assert.equal(y, 101)
    })

    it('works without a given BrowserWindow', () => {
      const { browserWindow, x, y } = menu.popup(100, 101)

      assert.equal(browserWindow.constructor.name, 'BrowserWindow')
      assert.equal(x, 100)
      assert.equal(y, 101)
    })

    it('works without a given BrowserWindow and 0 options', () => {
      const { browserWindow, x, y } = menu.popup(0, 1)

      assert.equal(browserWindow.constructor.name, 'BrowserWindow')
      assert.equal(x, 0)
      assert.equal(y, 1)
    })

    it('works with a given BrowserWindow and no options', () => {
      const { browserWindow, x, y } = menu.popup(w, 100, 101)

      assert.equal(browserWindow, w)
      assert.equal(x, 100)
      assert.equal(y, 101)
    })

    it('calls the callback', (done) => {
      menu.popup({}, () => done())
      menu.closePopup()
    })

    it('works with old style', (done) => {
      menu.popup(w, 100, 101, () => done())
      menu.closePopup()
    })
  })

  describe('Menu.setApplicationMenu', () => {
    it('sets a menu', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: '1'
        }, {
          label: '2'
        }
      ])
      Menu.setApplicationMenu(menu)
      assert.notEqual(Menu.getApplicationMenu(), null)
    })

    it('unsets a menu with null', () => {
      Menu.setApplicationMenu(null)
      assert.equal(Menu.getApplicationMenu(), null)
    })
  })

  describe('MenuItem.click', () => {
    it('should be called with the item object passed', function (done) {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          click: function (item) {
            assert.equal(item.constructor.name, 'MenuItem')
            assert.equal(item.label, 'text')
            done()
          }
        }
      ])
      menu.delegate.executeCommand({}, menu.items[0].commandId)
    })
  })

  describe('MenuItem with checked property', () => {
    it('clicking an checkbox item should flip the checked property', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          type: 'checkbox'
        }
      ])
      assert.equal(menu.items[0].checked, false)
      menu.delegate.executeCommand({}, menu.items[0].commandId)
      assert.equal(menu.items[0].checked, true)
    })

    it('clicking an radio item should always make checked property true', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          type: 'radio'
        }
      ])
      menu.delegate.executeCommand({}, menu.items[0].commandId)
      assert.equal(menu.items[0].checked, true)
      menu.delegate.executeCommand({}, menu.items[0].commandId)
      assert.equal(menu.items[0].checked, true)
    })

    it('at least have one item checked in each group', () => {
      const template = []
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
      const menu = Menu.buildFromTemplate(template)
      menu.delegate.menuWillShow()
      assert.equal(menu.items[0].checked, true)
      assert.equal(menu.items[12].checked, true)
    })

    it('should assign groupId automatically', () => {
      const template = []
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
      const menu = Menu.buildFromTemplate(template)
      const groupId = menu.items[0].groupId
      for (let i = 0; i <= 10; i++) {
        assert.equal(menu.items[i].groupId, groupId)
      }
      for (let i = 12; i <= 20; i++) {
        assert.equal(menu.items[i].groupId, groupId + 1)
      }
    })

    it("setting 'checked' should flip other items' 'checked' property", () => {
      const template = []
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

      const menu = Menu.buildFromTemplate(template)
      for (let i = 0; i <= 10; i++) {
        assert.equal(menu.items[i].checked, false)
      }
      menu.items[0].checked = true
      assert.equal(menu.items[0].checked, true)
      for (let i = 1; i <= 10; i++) {
        assert.equal(menu.items[i].checked, false)
      }
      menu.items[10].checked = true
      assert.equal(menu.items[10].checked, true)
      for (let i = 0; i <= 9; i++) {
        assert.equal(menu.items[i].checked, false)
      }
      for (let i = 12; i <= 20; i++) {
        assert.equal(menu.items[i].checked, false)
      }
      menu.items[12].checked = true
      assert.equal(menu.items[10].checked, true)
      for (let i = 0; i <= 9; i++) {
        assert.equal(menu.items[i].checked, false)
      }
      assert.equal(menu.items[12].checked, true)
      for (let i = 13; i <= 20; i++) {
        assert.equal(menu.items[i].checked, false)
      }
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
        Menu.buildFromTemplate([
          {
            label: 'text',
            type: 'not-a-type'
          }
        ])
      }, /Unknown menu item type: not-a-type/)
    })
  })

  describe('MenuItem with submenu type and missing submenu', () => {
    it('throws an exception', () => {
      assert.throws(() => {
        Menu.buildFromTemplate([
          {
            label: 'text',
            type: 'submenu'
          }
        ])
      }, /Invalid submenu/)
    })
  })

  describe('MenuItem role', () => {
    it('includes a default label and accelerator', () => {
      let item = new MenuItem({role: 'close'})
      assert.equal(item.label, process.platform === 'darwin' ? 'Close Window' : 'Close')
      assert.equal(item.accelerator, undefined)
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+W')

      item = new MenuItem({role: 'close', label: 'Other', accelerator: 'D'})
      assert.equal(item.label, 'Other')
      assert.equal(item.accelerator, 'D')
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+W')

      item = new MenuItem({role: 'help'})
      assert.equal(item.label, 'Help')
      assert.equal(item.accelerator, undefined)
      assert.equal(item.getDefaultRoleAccelerator(), undefined)

      item = new MenuItem({role: 'hide'})
      assert.equal(item.label, 'Hide Electron Test')
      assert.equal(item.accelerator, undefined)
      assert.equal(item.getDefaultRoleAccelerator(), 'Command+H')

      item = new MenuItem({role: 'undo'})
      assert.equal(item.label, 'Undo')
      assert.equal(item.accelerator, undefined)
      assert.equal(item.getDefaultRoleAccelerator(), 'CommandOrControl+Z')

      item = new MenuItem({role: 'redo'})
      assert.equal(item.label, 'Redo')
      assert.equal(item.accelerator, undefined)
      assert.equal(item.getDefaultRoleAccelerator(), process.platform === 'win32' ? 'Control+Y' : 'Shift+CommandOrControl+Z')
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
      const item = new MenuItem({role: 'editMenu', submenu: [{role: 'close'}]})
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
      const item = new MenuItem({role: 'windowMenu', submenu: [{role: 'copy'}]})
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
