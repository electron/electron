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

  constructor (options) {
    super()

    if (options == null) {
      throw new Error('Must specify options object as first argument')
    }

    let {items, escapeItem} = options

    // FIXME Support array as first argument, remove in 2.0
    if (Array.isArray(options)) {
      items = options
      escapeItem = null
    }

    if (!Array.isArray(items)) {
      items = []
    }

    this.changeListener = (item) => {
      this.emit('change', item.id, item.type)
    }

    this.windowListeners = {}
    this.items = {}
    this.ordereredItems = []
    this.escapeItem = escapeItem

    const registerItem = (item) => {
      this.items[item.id] = item
      item.on('change', this.changeListener)
      if (item.child instanceof TouchBar) {
        item.child.ordereredItems.forEach(registerItem)
      }
    }
    items.forEach((item) => {
      if (!(item instanceof TouchBarItem)) {
        throw new Error('Each item must be an instance of TouchBarItem')
      }
      this.ordereredItems.push(item)
      registerItem(item)
    })
  }

  set escapeItem (item) {
    if (item != null && !(item instanceof TouchBarItem)) {
      throw new Error('Escape item must be an instance of TouchBarItem')
    }
    if (this.escapeItem != null) {
      this.escapeItem.removeListener('change', this.changeListener)
    }
    this._escapeItem = item
    if (this.escapeItem != null) {
      this.escapeItem.on('change', this.changeListener)
    }
    this.emit('escape-item-change', item)
  }

  get escapeItem () {
    return this._escapeItem
  }

  _addToWindow (window) {
    const {id} = window

    // Already added to window
    if (this.windowListeners.hasOwnProperty(id)) return

    window._touchBar = this

    const changeListener = (itemID) => {
      window._refreshTouchBarItem(itemID)
    }
    this.on('change', changeListener)

    const escapeItemListener = (item) => {
      window._setEscapeTouchBarItem(item != null ? item : {})
    }
    this.on('escape-item-change', escapeItemListener)

    const interactionListener = (event, itemID, details) => {
      let item = this.items[itemID]
      if (item == null && this.escapeItem != null && this.escapeItem.id === itemID) {
        item = this.escapeItem
      }
      if (item != null && item.onInteraction != null) {
        item.onInteraction(details)
      }
    }
    window.on('-touch-bar-interaction', interactionListener)

    const removeListeners = () => {
      this.removeListener('change', changeListener)
      this.removeListener('escape-item-change', escapeItemListener)
      window.removeListener('-touch-bar-interaction', interactionListener)
      window.removeListener('closed', removeListeners)
      window._touchBar = null
      delete this.windowListeners[id]
    }
    window.once('closed', removeListeners)
    this.windowListeners[id] = removeListeners

    window._setTouchBarItems(this.ordereredItems)
    escapeItemListener(this.escapeItem)
  }

  _removeFromWindow (window) {
    const removeListeners = this.windowListeners[window.id]
    if (removeListeners != null) removeListeners()
  }
}

class TouchBarItem extends EventEmitter {
  constructor () {
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
        this.emit('change', this)
      },
      enumerable: true
    })
  }
}

TouchBar.TouchBarButton = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'button'
    const {click, icon, iconPosition, label, backgroundColor} = config
    this._addLiveProperty('label', label)
    this._addLiveProperty('backgroundColor', backgroundColor)
    this._addLiveProperty('icon', icon)
    this._addLiveProperty('iconPosition', iconPosition)
    if (typeof click === 'function') {
      this.onInteraction = () => {
        config.click()
      }
    }
  }
}

TouchBar.TouchBarColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
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

TouchBar.TouchBarGroup = class TouchBarGroup extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'group'
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.child)
    }
  }
}

TouchBar.TouchBarLabel = class TouchBarLabel extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'label'
    this._addLiveProperty('label', config.label)
    this._addLiveProperty('textColor', config.textColor)
  }
}

TouchBar.TouchBarPopover = class TouchBarPopover extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'popover'
    this._addLiveProperty('label', config.label)
    this._addLiveProperty('icon', config.icon)
    this.showCloseButton = config.showCloseButton
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.child)
    }
    this.child.ordereredItems.forEach((item) => {
      item._popover = item._popover || []
      if (!item._popover.includes(this.id)) item._popover.push(this.id)
    })
  }
}

TouchBar.TouchBarSlider = class TouchBarSlider extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
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

TouchBar.TouchBarSpacer = class TouchBarSpacer extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'spacer'
    this.size = config.size
  }
}

TouchBar.TouchBarSegmentedControl = class TouchBarSegmentedControl extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    const {segmentStyle, segments, selectedIndex, change, mode} = config
    this.type = 'segmented_control'
    this._addLiveProperty('segmentStyle', segmentStyle)
    this._addLiveProperty('segments', segments || [])
    this._addLiveProperty('selectedIndex', selectedIndex)
    this._addLiveProperty('mode', mode)

    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        this._selectedIndex = details.selectedIndex
        change(details.selectedIndex, details.isSelected)
      }
    }
  }
}

TouchBar.TouchBarScrubber = class TouchBarScrubber extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    const {items, selectedStyle, overlayStyle, showArrowButtons, continuous, mode} = config
    let {select, highlight} = config
    this.type = 'scrubber'
    this._addLiveProperty('items', items)
    this._addLiveProperty('selectedStyle', selectedStyle || null)
    this._addLiveProperty('overlayStyle', overlayStyle || null)
    this._addLiveProperty('showArrowButtons', showArrowButtons || false)
    this._addLiveProperty('mode', mode || 'free')
    this._addLiveProperty('continuous', typeof continuous === 'undefined' ? true : continuous)

    if (typeof select === 'function' || typeof highlight === 'function') {
      if (select == null) select = () => {}
      if (highlight == null) highlight = () => {}
      this.onInteraction = (details) => {
        if (details.type === 'select') {
          select(details.selectedIndex)
        } else if (details.type === 'highlight') {
          highlight(details.highlightedIndex)
        }
      }
    }
  }
}

module.exports = TouchBar
