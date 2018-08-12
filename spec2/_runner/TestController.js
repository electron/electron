const glob = require('glob')
const path = require('path')

const RunnerPool = require('./runners/RunnerPool')
const { RunnerType } = require('./runners/constants')
const { getTopLevelSuites, setCurrentFile } = require('./shared')

module.exports = class TestController {
  constructor() {
    this.pool = new RunnerPool()
  }

  glob(g) {
    return new Promise((resolve, reject) => {
      glob(g, { cwd: path.resolve(__dirname, '..') }, (err, matches) => {
        if (err) return reject(err)
        resolve(matches)
      })
    })
  }

  async registerTests() {
    const typesArg = process.argv.find(
      arg => arg.startsWith('--types=')
    )
    const types = ['browser', 'common', 'renderer']
    if (typesArg) {
      types = typesArg.split(',').filter(t => types.includes(t))
    }

    const files = []
    for (const t of types) {
      files.push(...await this.glob(`${t}/**/*.spec.js`))
    }

    for (const file of files) {
      const absolutePath = path.resolve(__dirname, '..', file)
      setCurrentFile(absolutePath)
      require(absolutePath)
    }
  }

  run() {
    for (const suite of getTopLevelSuites()) {
      this.runSuite(suite, [suite.name]);
    }
    this.pool.endWhenEmpty()
  }

  runSuite(suite, token) {
    for (const child of suite.children) {
      this.runSuite(child, token.concat([child.name]))
    }
    for (const test of suite.tests) {
      this.runTest(test, token.concat([test.name]))
    }
  }

  runTest(test, token) {
    this.pool.addToQueue(RunnerType.MAIN, test.fileName, token)
  }
}