const BaseRunner = require('./Runner')
const { RunnerType } = require('./constants')

module.exports = class RendererRunner extends BaseRunner {
  constructor(...args) {
    super(RunnerType.RENDERER, ...args)
    this.ready = true
  }
}
