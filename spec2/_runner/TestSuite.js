const { getCurrentFile, getTopLevelSuites, getSuiteChain } = require('./shared')

module.exports = class TestSuite {
  constructor(name, describer) {
    if (getSuiteChain().length > 0) {
      getSuiteChain()[getSuiteChain().length - 1].addChild(this)
    } else {
      getTopLevelSuites().push(this)
    }

    this.name = name
    this.fileName = getCurrentFile()

    this.tests = []
    this.children = []

    getSuiteChain().push(this)
    const dReturn = describer()
    getSuiteChain().pop()

    if (typeof dReturn !== 'undefined') {
      throw new Error('A describe method returned a value, all describes should be syncronous and return undefined')
    }
  }

  has(name) {
    return this.children.some(child => child.name === name)
      || this.tests.find(test => test.name === name)
  }

  addChild(suite) {
    if (this.has(suite.name)) {
      console.error(`ERROR: You registered two things with the same name in the same heirachy position: "${suite.name}"`)
      process.exit(1)
    }
    this.children.push(suite)
  }

  addTest(test) {
    if (this.has(test.name)) {
      console.error(`ERROR: You registered two things with the same name in the same heirachy position: "${test.name}"`)
      process.exit(1)
    }
    this.tests.push(test)
  }

  async run(controller, token) {
    for (const childSuite of this.children) {
      await controller.runSuite(childSuite, token)
    }
    for (const test of this.tests) {
      await test.run(controller, controller.tokenForTest(test, token))
    }
  }
}