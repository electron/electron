const assert = require('assert')
const path = require('path')
const {BrowserWindow, TouchBar} = require('electron').remote
const {closeWindow} = require('./window-helpers')

const {TouchBarButton, TouchBarColorPicker, TouchBarGroup} = TouchBar
const {TouchBarLabel, TouchBarPopover, TouchBarScrubber, TouchBarSegmentedControl, TouchBarSlider, TouchBarSpacer} = TouchBar

describe('TouchBar module', () => {
  it('throws an error when created without an options object', () => {
    assert.throws(() => {
      const touchBar = new TouchBar()
      touchBar.toString()
    }, /Must specify options object as first argument/)
  })

  it('throws an error when created with invalid items', () => {
    assert.throws(() => {
      const touchBar = new TouchBar({items: [1, true, {}, []]})
      touchBar.toString()
    }, /Each item must be an instance of TouchBarItem/)
  })

  it('throws an error when an invalid escape item is set', () => {
    assert.throws(() => {
      const touchBar = new TouchBar({items: [], escapeItem: 'esc'})
      touchBar.toString()
    }, /Escape item must be an instance of TouchBarItem/)

    assert.throws(() => {
      const touchBar = new TouchBar({items: []})
      touchBar.escapeItem = 'esc'
    }, /Escape item must be an instance of TouchBarItem/)
  })

  describe('BrowserWindow behavior', () => {
    let window

    beforeEach(() => {
      window = new BrowserWindow()
    })

    afterEach(() => {
      window.setTouchBar(null)
      return closeWindow(window).then(() => { window = null })
    })

    it('can be added to and removed from a window', () => {
      const label = new TouchBarLabel({label: 'bar'})
      const touchBar = new TouchBar([
        new TouchBarButton({label: 'foo', backgroundColor: '#F00', click: () => {}}),
        new TouchBarButton({
          icon: path.join(__dirname, 'fixtures', 'assets', 'logo.png'),
          iconPosition: 'right',
          click: () => {}
        }),
        new TouchBarColorPicker({selectedColor: '#F00', change: () => {}}),
        new TouchBarGroup({items: new TouchBar([new TouchBarLabel({label: 'hello'})])}),
        label,
        new TouchBarPopover({items: new TouchBar([new TouchBarButton({label: 'pop'})])}),
        new TouchBarSlider({label: 'slide', value: 5, minValue: 2, maxValue: 75, change: () => {}}),
        new TouchBarSpacer({size: 'large'}),
        new TouchBarSegmentedControl({
          segmentStyle: 'capsule',
          segments: [{label: 'baz', enabled: false}],
          selectedIndex: 5
        }),
        new TouchBarSegmentedControl({segments: []}),
        new TouchBarScrubber({
          items: [{label: 'foo'}, {label: 'bar'}, {label: 'baz'}],
          selectedStyle: 'outline',
          mode: 'fixed',
          showArrowButtons: true
        })
      ])
      const escapeButton = new TouchBarButton({label: 'foo'})
      window.setTouchBar(touchBar)
      touchBar.escapeItem = escapeButton
      label.label = 'baz'
      escapeButton.label = 'hello'
      window.setTouchBar()
      window.setTouchBar(new TouchBar([new TouchBarLabel({label: 'two'})]))
      touchBar.escapeItem = null
    })

    it('calls the callback on the items when a window interaction event fires', (done) => {
      const button = new TouchBarButton({
        label: 'bar',
        click: () => {
          done()
        }
      })
      const touchBar = new TouchBar({items: [button]})
      window.setTouchBar(touchBar)
      window.emit('-touch-bar-interaction', {}, button.id)
    })

    it('calls the callback on the escape item when a window interaction event fires', (done) => {
      const button = new TouchBarButton({
        label: 'bar',
        click: () => {
          done()
        }
      })
      const touchBar = new TouchBar({escapeItem: button})
      window.setTouchBar(touchBar)
      window.emit('-touch-bar-interaction', {}, button.id)
    })
  })
})
