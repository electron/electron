const assert = require('assert')
const {BrowserWindow, TouchBar} = require('electron').remote
const {closeWindow} = require('./window-helpers')

const {TouchBarButton, TouchBarColorPicker, TouchBarGroup} = TouchBar
const {TouchBarLabel, TouchBarPopover, TouchBarSlider, TouchBarSpacer} = TouchBar

describe('TouchBar module', function () {
  it('throws an error when created without an items array', function () {
    assert.throws(() => {
      const touchBar = new TouchBar()
      touchBar.toString()
    }, /Must specify items array as first argument/)
  })

  it('throws an error when created with invalid items', function () {
    assert.throws(() => {
      const touchBar = new TouchBar([1, true, {}, []])
      touchBar.toString()
    }, /Each item must be an instance of TouchBarItem/)
  })

  describe('BrowserWindow behavior', function () {
    let window

    beforeEach(function () {
      window = new BrowserWindow()
    })

    afterEach(function () {
      window.setTouchBar(null)
      return closeWindow(window).then(function () { window = null })
    })

    it('can be added to and removed from a window', function () {
      const label = new TouchBarLabel({label: 'bar'})
      const touchBar = new TouchBar([
        new TouchBarButton({label: 'foo', backgroundColor: '#F00', click: () => {}}),
        new TouchBarColorPicker({selectedColor: '#F00', change: () => {}}),
        new TouchBarGroup({items: new TouchBar([new TouchBarLabel({label: 'hello'})])}),
        label,
        new TouchBarPopover({items: new TouchBar([new TouchBarButton({label: 'pop'})])}),
        new TouchBarSlider({label: 'slide', value: 5, minValue: 2, maxValue: 75, change: () => {}}),
        new TouchBarSpacer({size: 'large'})
      ])
      window.setTouchBar(touchBar)
      label.label = 'baz'
      window.setTouchBar()
      window.setTouchBar(new TouchBar([new TouchBarLabel({label: 'two'})]))
    })
  })
})
