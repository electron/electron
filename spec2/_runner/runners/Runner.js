const debug = require('debug')

let runnerId = 1

module.exports = class Runner {
  constructor(type, pool) {
    this.d = debug(`etest:runner:${runnerId++}`)
    this.type = type
    this.pool = pool
    this.ready = false

    this.d('this new runner has type', type)
  }

  writeLog(...message) {
    this.d(...message)
  }

  stop() {
    this.d('stop is not implemented')
  }
}