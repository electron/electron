const {EventEmitter} = require('events')

let itemIdIncrementor = 1

class TouchBar extends EventEmitter {

  // Bind a touch bar to a window
  static _setOnWindow (touchBar, window) {
    if (window._touchBar != null) {
      window._touchBar._removeFromWindow(window)
    }

    if (touchBar == null) {
      window._setTouchBarItems([])
      return
    }

    if (Array.isArray(touchBar)) {
      touchBar = new TouchBar(touchBar)
    }
    touchBar._addToWindow(window)
  }

  constructor (items) {
    super()

    if (!Array.isArray(items)) {
      throw new Error('The items object provided has to be an array')
    }

    this.windowListeners = {}
    this.items = {}
    this.ordereredItems = []
    const registerItem = (item) => {
      this.items[item.id] = item
      item.on('change', () => {
        this.emit('change', item.id, item.type)
      })
      if (item.child instanceof TouchBar) {
        item.child.ordereredItems.forEach(registerItem)
      }
    }
    items.forEach((item) => {
      this.ordereredItems.push(item)
      if (!(item instanceof TouchBarItem)) {
        throw new Error('Each item must be an instance of a TouchBarItem')
      }
      registerItem(item)
    })
  }

  // Called by BrowserWindow.setTouchBar
  _addToWindow (window) {
    const {id} = window

    // Already added to window
    if (this.windowListeners.hasOwnProperty(id)) return

    window._touchBar = this

    const changeListener = (itemID) => {
      window._refreshTouchBarItem(itemID)
    }
    this.on('change', changeListener)

    const interactionListener = (event, itemID, details) => {
      const item = this.items[itemID]
      if (item != null && item.onInteraction != null) {
        item.onInteraction(details)
      }
    }
    window.on('-touch-bar-interaction', interactionListener)

    const removeListeners = () => {
      this.removeListener('change', changeListener)
      window.removeListener('-touch-bar-interaction', interactionListener)
      window.removeListener('closed', removeListeners)
      window._touchBar = null
      delete this.windowListeners[id]
    }
    window.once('closed', removeListeners)
    this.windowListeners[id] = removeListeners

    window._setTouchBarItems(this.ordereredItems)
  }

  // Called by BrowserWindow.setTouchBar
  _removeFromWindow (window) {
    const removeListeners = this.windowListeners[window.id]
    if (removeListeners != null) removeListeners()
  }
}

class TouchBarItem extends EventEmitter {
  constructor (config) {
    super()
    this.id = `${itemIdIncrementor++}`
  }
}

TouchBar.Button = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'button'
    this.label = config.label
    this.backgroundColor = config.backgroundColor
    this.labelColor = config.labelColor
    this.onInteraction = config.click
  }
}

TouchBar.ColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'colorpicker'

    const {change} = config
    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        change(details.color)
      }
    }
  }
}

TouchBar.Group = class TouchBarGroup extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'group'
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.items)
    }
  }
}

TouchBar.Label = class TouchBarLabel extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'label'
    this._label = config.label
  }

  set label (newLabel) {
    this._label = newLabel
    this.emit('change')
  }

  get label () {
    return this._label
  }
}

TouchBar.PopOver = class TouchBarPopOver extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'popover'
    this.label = config.label
    this.showCloseButton = config.showCloseButton
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.items)
    }
  }
}

TouchBar.Slider = class TouchBarSlider extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'slider'
    this.minValue = config.minValue
    this.maxValue = config.maxValue
    this.initialValue = config.initialValue

    const {change} = config
    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        change(details.value)
      }
    }
  }
}

module.exports = TouchBar
