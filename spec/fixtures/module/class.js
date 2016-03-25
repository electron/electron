'use strict'

let value = 'old'

class BaseClass {
  method () {
    return 'method'
  }

  get readonly () {
    return 'readonly'
  }

  get value () {
    return value
  }

  set value (val) {
    value = val
  }
}

class DerivedClass extends BaseClass {
}

module.exports = {
  base: new BaseClass,
  derived: new DerivedClass,
}
