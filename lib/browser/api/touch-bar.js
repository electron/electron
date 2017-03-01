const {EventEmitter} = require('events')

let nextItemID = 1

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
    this.id = `${nextItemID++}`
  }

  _addLiveProperty (name, initialValue) {
    const privateName = `_${name}`
    this[privateName] = initialValue
    Object.defineProperty(this, name, {
      get: function () {
        return this[privateName]
      },
      set: function (value) {
        this[privateName] = value
        this.emit('change')
      },
      enumerable: true
    })
  }
}

TouchBar.Button = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'button'
    const {click, label, backgroundColor} = config
    this._addLiveProperty('label', label)
    this._addLiveProperty('backgroundColor', backgroundColor)
    if (typeof click === 'function') {
      this.onInteraction = config.click
    }
  }
}

TouchBar.ColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'colorpicker'
    const {availableColors, change, selectedColor} = config
    this._addLiveProperty('availableColors', availableColors)
    this._addLiveProperty('selectedColor', selectedColor)

    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        this._selectedColor = details.color
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
    this._addLiveProperty('label', config.label)
  }
}

TouchBar.Spacer = class TouchBarSpacer extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'spacer'
    this._addLiveProperty('size', config.size)
  }
}

TouchBar.Popover = class TouchBarPopover extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'popover'
    this._addLiveProperty('label', config.label)
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
    const {change, label, minValue, maxValue, value} = config
    this._addLiveProperty('label', label)
    this._addLiveProperty('minValue', minValue)
    this._addLiveProperty('maxValue', maxValue)
    this._addLiveProperty('value', value)

    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        this._value = details.value
        change(details.value)
      }
    }
  }
}

module.exports = TouchBar
