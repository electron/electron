const d = require('debug')('etest:runner-pool')

const MainRunner = require('./MainRunner')
const RendererRunner = require('./RendererRunner')

const MAX_RUNNERS_OF_TYPE = 1

module.exports = class RunnerPool {
  constructor() {
    this.runners = []
    this.queue = []
    this._endWhenEmpty = false

    for (let i = 0; i < MAX_RUNNERS_OF_TYPE; i++) {
      this.runners.push(new MainRunner(this))
      this.runners.push(new RendererRunner(this))
    }

    d(`registered ${this.runners.length} runners`)
  }

  addToQueue(type, file, testPath) {
    this.queue.push({
      type,
      file,
      testPath
    })
    this.clearQueue()
  }

  getNextItemOfType(type) {
    let item
    this.queue = this.queue.filter((tItem) => {
      if (item) return true
      if (tItem.type === type) {
        item = tItem
        return false
      }
      return true
    })
    return item
  }

  clearQueue() {
    for (const runner of this.runners) {
      if (!runner.ready) continue
      const eligibleItem = this.getNextItemOfType(runner.type)
      if (eligibleItem) {
        runner.runTest(eligibleItem.file, eligibleItem.testPath)
        runner.ready = false
      }
    }
  }

  onRunnerReady(runner) {
    runner.ready = true
    if (this._endWhenEmpty) {
      this.endWhenEmpty()
    }
    this.clearQueue()
  }

  endWhenEmpty() {
    if (this.queue.length === 0 && !this.runners.find(runner => !runner.ready)) {
      for (const runner of this.runners) {
        runner.stop()
      }
    }
    this._endWhenEmpty = true
  }
}