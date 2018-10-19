const assert = require('assert')

const {ipcRenderer, remote} = require('electron')
const {BrowserWindow, Menu, MenuItem} = remote
const {sortMenuItems} = require('../lib/browser/api/menu-utils')
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
      const template = [{label: 'text', submenu: [{label: 'sub'}]}]
      const result = ipcRenderer.sendSync('eval', `const template = [{label: 'text', submenu: [{label: 'sub'}]}]\nrequire('electron').Menu.buildFromTemplate(template)\ntemplate`)
      assert.deepStrictEqual(result, template)
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

    describe('Menu sorting and building', () => {
      describe('sorts groups', () => {
        it('does a simple sort', () => {
          const items = [
            {
              label: 'two',
              id: '2',
              afterGroupContaining: ['1'] },
            { type: 'separator' },
            {
              id: '1',
              label: 'one'
            }
          ]

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['1']
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it('resolves cycles by ignoring things that conflict', () => {
          const items = [
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['1']
            },
            { type: 'separator' },
            {
              id: '1',
              label: 'one',
              afterGroupContaining: ['2']
            }
          ]

          const expected = [
            {
              id: '1',
              label: 'one',
              afterGroupContaining: ['2']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['1']
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it('ignores references to commands that do not exist', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['does-not-exist']
            }
          ]

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['does-not-exist']
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it('only respects the first matching [before|after]GroupContaining rule in a given group', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              beforeGroupContaining: ['1']
            },
            {
              id: '4',
              label: 'four',
              afterGroupContaining: ['2']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ]

          const expected = [
            {
              id: '3',
              label: 'three',
              beforeGroupContaining: ['1']
            },
            {
              id: '4',
              label: 'four',
              afterGroupContaining: ['2']
            },
            { type: 'separator' },
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })
      })

      describe('moves an item to a different group by merging groups', () => {
        it('can move a group of one item', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            { type: 'separator' }
          ]

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it("moves all items in the moving item's group", () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            {
              id: '4',
              label: 'four'
            },
            { type: 'separator' }
          ]

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            {
              id: '4',
              label: 'four'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it("ignores positions relative to commands that don't exist", () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['does-not-exist']
            },
            {
              id: '4',
              label: 'four',
              after: ['1']
            },
            { type: 'separator' }
          ]

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['does-not-exist']
            },
            {
              id: '4',
              label: 'four',
              after: ['1']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it('can handle recursive group merging', () => {
          const items = [
            {
              id: '1',
              label: 'one',
              after: ['3']
            },
            {
              id: '2',
              label: 'two',
              before: ['1']
            },
            {
              id: '3',
              label: 'three'
            }
          ]

          const expected = [
            {
              id: '3',
              label: 'three'
            },
            {
              id: '2',
              label: 'two',
              before: ['1']
            },
            {
              id: '1',
              label: 'one',
              after: ['3']
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it('can merge multiple groups when given a list of before/after commands', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1', '2']
            }
          ]

          const expected = [
            {
              id: '2',
              label: 'two'
            },
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1', '2']
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })

        it('can merge multiple groups based on both before/after commands', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1'],
              before: ['2']
            }
          ]

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1'],
              before: ['2']
            },
            {
              id: '2',
              label: 'two'
            }
          ]

          assert.deepEqual(sortMenuItems(items), expected)
        })
      })

      it('should position before existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two'
          }, {
            id: '3',
            label: 'three'
          }, {
            id: '1',
            label: 'one',
            before: ['2']
          }
        ])

        assert.equal(menu.items[0].label, 'one')
        assert.equal(menu.items[1].label, 'two')
        assert.equal(menu.items[2].label, 'three')
      })

      it('should position after existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two',
            after: ['1']
          },
          {
            id: '1',
            label: 'one'
          }, {
            id: '3',
            label: 'three'
          }
        ])

        assert.equal(menu.items[0].label, 'one')
        assert.equal(menu.items[1].label, 'two')
        assert.equal(menu.items[2].label, 'three')
      })

      it('should filter excess menu separators', () => {
        const menuOne = Menu.buildFromTemplate([
          {
            type: 'separator'
          }, {
            label: 'a'
          }, {
            label: 'b'
          }, {
            label: 'c'
          }, {
            type: 'separator'
          }
        ])

        assert.equal(menuOne.items.length, 3)
        assert.equal(menuOne.items[0].label, 'a')
        assert.equal(menuOne.items[1].label, 'b')
        assert.equal(menuOne.items[2].label, 'c')

        const menuTwo = Menu.buildFromTemplate([
          {
            type: 'separator'
          }, {
            type: 'separator'
          }, {
            label: 'a'
          }, {
            label: 'b'
          }, {
            label: 'c'
          }, {
            type: 'separator'
          }, {
            type: 'separator'
          }
        ])

        assert.equal(menuTwo.items.length, 3)
        assert.equal(menuTwo.items[0].label, 'a')
        assert.equal(menuTwo.items[1].label, 'b')
        assert.equal(menuTwo.items[2].label, 'c')
      })

      it('should continue inserting items at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two'
          }, {
            id: '3',
            label: 'three'
          }, {
            id: '4',
            label: 'four'
          }, {
            id: '5',
            label: 'five'
          }, {
            id: '1',
            label: 'one',
            before: ['2']
          }
        ])

        assert.equal(menu.items[0].label, 'one')
        assert.equal(menu.items[1].label, 'two')
        assert.equal(menu.items[2].label, 'three')
        assert.equal(menu.items[3].label, 'four')
        assert.equal(menu.items[4].label, 'five')
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
              accelerator: 'ControlCommandF',
              id: 'fullScreen'
            }
          ]
        }
      ])
      const fsc = menu.getMenuItemById('fullScreen')
      assert.equal(menu.items[0].submenu.items[0], fsc)
    })

    it('should return the separator with the given id', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'Item 1',
          id: 'item_1'
        },
        {
          id: 'separator',
          type: 'separator'
        },
        {
          label: 'Item 2',
          id: 'item_2'
        }
      ])
      const separator = menu.getMenuItemById('separator')
      assert.equal(typeof separator, 'object')
      assert.equal(separator, menu.items[1])
    })
  })

  describe('Menu.insert', () => {
    it('should store item in @items by its index', () => {
      const menu = Menu.buildFromTemplate([
        {label: '1'},
        {label: '2'},
        {label: '3'}
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
        {label: '1'},
        {label: '2'},
        {label: '3'}
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
        {label: '1'},
        {label: '2'},
        {label: '3'}
      ])
    })

    afterEach(() => {
      menu.closePopup()
      menu.closePopup(w)
      return closeWindow(w).then(() => { w = null })
    })

    it('throws an error if options is not an object', () => {
      assert.throws(() => {
        menu.popup()
      }, /Options must be an object/)
    })

    it('should emit menu-will-show event', (done) => {
      menu.on('menu-will-show', () => { done() })
      menu.popup({window: w})
    })

    it('should emit menu-will-close event', (done) => {
      menu.on('menu-will-close', () => { done() })
      menu.popup({window: w})
      menu.closePopup()
    })

    it('returns immediately', () => {
      const input = {window: w, x: 100, y: 101}
      const output = menu.popup(input)
      assert.equal(output.x, input.x)
      assert.equal(output.y, input.y)
      assert.equal(output.browserWindow, input.window)
    })

    it('works without a given BrowserWindow and options', () => {
      const {browserWindow, x, y} = menu.popup({x: 100, y: 101})

      assert.equal(browserWindow.constructor.name, 'BrowserWindow')
      assert.equal(x, 100)
      assert.equal(y, 101)
    })

    it('works with a given BrowserWindow, options and callback', (done) => {
      const {x, y} = menu.popup({
        window: w,
        x: 100,
        y: 101,
        callback: () => done()
      })

      assert.equal(x, 100)
      assert.equal(y, 101)
      menu.closePopup()
    })

    it('works with a given BrowserWindow, no options, and a callback', (done) => {
      menu.popup({window: w, callback: () => done()})
      menu.closePopup()
    })
  })

  describe('Menu.setApplicationMenu', () => {
    it('sets a menu', () => {
      const menu = Menu.buildFromTemplate([
        {label: '1'},
        {label: '2'}
      ])

      Menu.setApplicationMenu(menu)
      assert.notEqual(Menu.getApplicationMenu(), null)
    })

    it('unsets a menu with null', () => {
      Menu.setApplicationMenu(null)
      assert.equal(Menu.getApplicationMenu(), null)
    })
  })
})
