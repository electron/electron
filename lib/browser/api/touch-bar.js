const {EventEmitter} = require('events')
const {app} = require('electron')

class TouchBar {
  constructor (items) {
    this.items = items;
    if (!Array.isArray(items)) {
      throw new Error('The items object provided has to be an array')
    }
    console.log(items)
    items.forEach((item) => {
      if (!item.id) {
        throw new Error('Each item must be an instance of a TouchBarItem')
      }
    })
  }

  toJSON () {
    return this.items.map((item) => item.toJSON ? item.toJSON() : item);
  }
}

let item_id_incrementor = 1
const item_event_handlers = {}

TouchBar._event = (id, ...args) => {
  const id_parts = id.split('.')
  const item_id = id_parts[id_parts.length - 1]
  if (item_event_handlers[item_id]) item_event_handlers[item_id](...args)
}

class TouchBarItem {
  constructor (config) {
    this.id = item_id_incrementor++
    const mConfig = Object.assign({}, config || {})
    Object.defineProperty(this, 'config', {
      configurable: false,
      enumerable: false,
      get: () => mConfig
    })
    this.config.id = `${this.config.id || this.id}`;
    this.config.type = 'button';
    if (typeof this.config !== 'object' || this.config === null) {
      throw new Error('Provided config must be a non-null object')
    }
  }

  toJSON () {
    return this.config
  }
}

TouchBar.Button = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'button';
    const click = config.click
    if (typeof click === 'function') {
      item_event_handlers[`${this.id}`] = click
    }
  }
}

TouchBar.ColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'colorpicker';
    const change = config.change
    if (typeof change === 'function') {
      item_event_handlers[`${this.id}`] = change
    }
  }
}

TouchBar.Label = class TouchBarLabel extends TouchBarItem {}

TouchBar.List = class TouchBarList extends TouchBarItem {}

TouchBar.Slider = class TouchBarSlider extends TouchBarItem {
  constructor (config) {
    super(config)
    const change = config.change
    if (typeof change === 'function') {
      item_event_handlers[this.id] = change
    }
  }
}

module.exports = TouchBar;
