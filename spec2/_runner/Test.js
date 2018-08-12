const { getSuiteChain, getCurrentFile } = require('./shared')
const { ResultType } = require('./runners/constants')

module.exports = class Test {
  constructor(name, testFn) {
    this.name = name
    this.testFn = testFn
    this.fileName = getCurrentFile()

    getSuiteChain()[getSuiteChain().length - 1].addTest(this)
  }

  async run(controller) {
    let finalized = false
    const errHandler = (err) => finalize(ResultType.FAIL, err)
    const finalize = (result, err) => {
      process.removeListener('unhandledPromiseRejection', errHandler)
      if (finalized) return
      finalized = true

      switch (result) {
        case ResultType.SKIP:
          return controller.registerSkip()
        case ResultType.FAIL:
          return controller.registerFail(err)
        case ResultType.PASS:
          return controller.registerPass()
      }
      controller.registerFail(new Error(`Bad test result returned: ${result}`))
    }

    if (this.skipper && await Promise.resolve(this.skipper)) {
      return finalize(ResultType.SKIP)
    }

    process.once('unhandledPromiseRejection', errHandler)
    try {
      await Promise.resolve(this.testFn())
    } catch (err) {
      return errHandler(err)
    }
    finalize(ResultType.PASS)
  }

  skipWhen(skipper) {
    this.skipper = skipper
  }

  skip() {
    this.skipper = () => true
  }
}