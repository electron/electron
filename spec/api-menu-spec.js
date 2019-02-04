const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { ipcRenderer, remote } = require('electron')
const { BrowserWindow, Menu, MenuItem } = remote
const { sortMenuItems } = require('../lib/browser/api/menu-utils')
const { closeWindow } = require('./window-helpers')

const { expect } = chai
chai.use(dirtyChai)

describe('Menu module', () => {
  describe('Menu.buildFromTemplate', () => {
    it('should be able to attach extra fields', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          extra: 'field'
        }
      ])
      expect(menu.items[0].extra).to.equal('field')
    })

    it('should be able to accept only MenuItems', () => {
      const menu = Menu.buildFromTemplate([
        new MenuItem({ label: 'one' }),
        new MenuItem({ label: 'two' })
      ])
      expect(menu.items[0].label).to.equal('one')
      expect(menu.items[1].label).to.equal('two')
    })

    it('should be able to accept only MenuItems in a submenu', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'one',
          submenu: [
            new MenuItem({ label: 'two' })
          ]
        }
      ])
      expect(menu.items[0].label).to.equal('one')
      expect(menu.items[0].submenu.items[0].label).to.equal('two')
    })

    it('should be able to accept MenuItems and plain objects', () => {
      const menu = Menu.buildFromTemplate([
        new MenuItem({ label: 'one' }),
        { label: 'two' }
      ])
      expect(menu.items[0].label).to.equal('one')
      expect(menu.items[1].label).to.equal('two')
    })

    it('does not modify the specified template', () => {
      const template = [{ label: 'text', submenu: [{ label: 'sub' }] }]
      const result = ipcRenderer.sendSync('eval', `const template = [{label: 'text', submenu: [{label: 'sub'}]}]\nrequire('electron').Menu.buildFromTemplate(template)\ntemplate`)
      expect(result).to.deep.equal(template)
    })

    it('does not throw exceptions for undefined/null values', () => {
      expect(() => {
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
      }).to.not.throw()
    })

    it('does throw exceptions for empty objects and null values', () => {
      expect(() => {
        Menu.buildFromTemplate([{}, null])
      }).to.throw(/Invalid template for MenuItem: must have at least one of label, role or type/)
    })

    it('does throw exception for object without role, label, or type attribute', () => {
      expect(() => {
        Menu.buildFromTemplate([{ 'visible': true }])
      }).to.throw(/Invalid template for MenuItem: must have at least one of label, role or type/)
    })
    it('does throw exception for undefined', () => {
      expect(() => {
        Menu.buildFromTemplate([undefined])
      }).to.throw(/Invalid template for MenuItem: must have at least one of label, role or type/)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
        })

        it('does a simple sort with MenuItems', () => {
          const firstItem = new MenuItem({ id: '1', label: 'one' })
          const secondItem = new MenuItem({
            label: 'two',
            id: '2',
            afterGroupContaining: ['1']
          })
          const sep = new MenuItem({ type: 'separator' })

          const items = [ secondItem, sep, firstItem ]
          const expected = [ firstItem, sep, secondItem ]

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

          expect(sortMenuItems(items)).to.deep.equal(expected)
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

        expect(menu.items[0].label).to.equal('one')
        expect(menu.items[1].label).to.equal('two')
        expect(menu.items[2].label).to.equal('three')
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

        expect(menu.items[0].label).to.equal('one')
        expect(menu.items[1].label).to.equal('two')
        expect(menu.items[2].label).to.equal('three')
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

        expect(menuOne.items).to.have.length(3)
        expect(menuOne.items[0].label).to.equal('a')
        expect(menuOne.items[1].label).to.equal('b')
        expect(menuOne.items[2].label).to.equal('c')

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

        expect(menuTwo.items).to.have.length(3)
        expect(menuTwo.items[0].label).to.equal('a')
        expect(menuTwo.items[1].label).to.equal('b')
        expect(menuTwo.items[2].label).to.equal('c')
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

        expect(menu.items[0].label).to.equal('one')
        expect(menu.items[1].label).to.equal('two')
        expect(menu.items[2].label).to.equal('three')
        expect(menu.items[3].label).to.equal('four')
        expect(menu.items[4].label).to.equal('five')
      })

      it('should continue inserting MenuItems at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          new MenuItem({
            id: '2',
            label: 'two'
          }), new MenuItem({
            id: '3',
            label: 'three'
          }), new MenuItem({
            id: '4',
            label: 'four'
          }), new MenuItem({
            id: '5',
            label: 'five'
          }), new MenuItem({
            id: '1',
            label: 'one',
            before: ['2']
          })
        ])

        expect(menu.items[0].label).to.equal('one')
        expect(menu.items[1].label).to.equal('two')
        expect(menu.items[2].label).to.equal('three')
        expect(menu.items[3].label).to.equal('four')
        expect(menu.items[4].label).to.equal('five')
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
      expect(menu.items[0].submenu.items[0]).to.equal(fsc)
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
      expect(separator).to.be.an('object')
      expect(separator).to.equal(menu.items[1])
    })
  })

  describe('Menu.insert', () => {
    it('should store item in @items by its index', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ])

      const item = new MenuItem({ label: 'inserted' })

      menu.insert(1, item)
      expect(menu.items[0].label).to.equal('1')
      expect(menu.items[1].label).to.equal('inserted')
      expect(menu.items[2].label).to.equal('2')
      expect(menu.items[3].label).to.equal('3')
    })
  })

  describe('Menu.append', () => {
    it('should add the item to the end of the menu', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ])

      const item = new MenuItem({ label: 'inserted' })
      menu.append(item)

      expect(menu.items[0].label).to.equal('1')
      expect(menu.items[1].label).to.equal('2')
      expect(menu.items[2].label).to.equal('3')
      expect(menu.items[3].label).to.equal('inserted')
    })
  })

  describe('Menu.popup', () => {
    let w = null
    let menu

    beforeEach(() => {
      w = new BrowserWindow({ show: false, width: 200, height: 200 })
      menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ])
    })

    afterEach(() => {
      menu.closePopup()
      menu.closePopup(w)
      return closeWindow(w).then(() => { w = null })
    })

    it('throws an error if options is not an object', () => {
      expect(() => {
        menu.popup('this is a string, not an object')
      }).to.throw(/Options must be an object/)
    })

    it('allows for options to be optional', () => {
      expect(() => {
        menu.popup({})
      }).to.not.throw()
    })

    it('should emit menu-will-show event', (done) => {
      menu.on('menu-will-show', () => { done() })
      menu.popup({ window: w })
    })

    it('should emit menu-will-close event', (done) => {
      menu.on('menu-will-close', () => { done() })
      menu.popup({ window: w })
      menu.closePopup()
    })

    it('returns immediately', () => {
      const input = { window: w, x: 100, y: 101 }
      const output = menu.popup(input)
      expect(output.x).to.equal(input.x)
      expect(output.y).to.equal(input.y)
      expect(output.browserWindow).to.equal(input.window)
    })

    it('works without a given BrowserWindow and options', () => {
      const { browserWindow, x, y } = menu.popup({ x: 100, y: 101 })

      expect(browserWindow.constructor.name).to.equal('BrowserWindow')
      expect(x).to.equal(100)
      expect(y).to.equal(101)
    })

    it('works with a given BrowserWindow, options and callback', (done) => {
      const { x, y } = menu.popup({
        window: w,
        x: 100,
        y: 101,
        callback: () => done()
      })

      expect(x).to.equal(100)
      expect(y).to.equal(101)
      menu.closePopup()
    })

    it('works with a given BrowserWindow, no options, and a callback', (done) => {
      menu.popup({ window: w, callback: () => done() })
      menu.closePopup()
    })
  })

  describe('Menu.setApplicationMenu', () => {
    it('sets a menu', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' }
      ])

      Menu.setApplicationMenu(menu)
      expect(Menu.getApplicationMenu()).to.not.be.null()
    })

    it('unsets a menu with null', () => {
      Menu.setApplicationMenu(null)
      expect(Menu.getApplicationMenu()).to.be.null()
    })
  })

  describe('menu accelerators', () => {
    let testFn = it
    try {
      // We have other tests that check if native modules work, if we fail to require
      // robotjs let's skip this test to avoid false negatives
      require('robotjs')
    } catch (err) {
      testFn = it.skip
    }
    const sendRobotjsKey = (key, modifiers = [], delay = 500) => {
      return new Promise((resolve, reject) => {
        require('robotjs').keyTap(key, modifiers)
        setTimeout(() => {
          resolve()
        }, delay)
      })
    }

    testFn('menu accelerators perform the specified action', async () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'Test',
          submenu: [
            {
              label: 'Test Item',
              accelerator: 'Ctrl+T',
              click: () => {
                // Test will succeed, only when the menu accelerator action
                // is triggered
                Promise.resolve()
              },
              id: 'test'
            }
          ]
        }
      ])
      Menu.setApplicationMenu(menu)
      expect(Menu.getApplicationMenu()).to.not.be.null()
      await sendRobotjsKey('t', 'control')
    })
  })
})
