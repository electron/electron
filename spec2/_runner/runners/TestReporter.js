require('colors')
const { AssertionError } = require('chai')
const builder = require('junit-report-builder')
const path = require('path')

const { ResultType } = require('./constants')

class TestReporter {
  constructor() {
    this.builder = builder.newBuilder()
    this.suites = new Map()
  }

  getSuiteForPath(testPath) {
    const key = testPath.slice(0, -1).join('#$#')
    if (!this.suites.has(key)) {
      this.suites.set(key, this.builder.testSuite().name(testPath.slice(0, -1).join(' ')))
    }
    return this.suites.get(key)
  }

  report(fileName, testPath, result, err) {
    const suite = this.getSuiteForPath(testPath)
    const testCase = suite.testCase().name(testPath[testPath.length - 1])

    switch (result) {
      case ResultType.PASS:
        break;
      case ResultType.SKIP:
        testCase.skipped()
        break;
    }

    console.log(`Test Result: ${this.nicePath(fileName).cyan}`)
    console.log(`        ${testPath.join(' --> ')}: ${this.niceResult(result)}`)
    if (err) {
      testCase.failure(err.message)
      testCase.stacktrace(err.stack)
      if (err.name === 'AssertionError') {
        console.error(err.stack.red)
      } else {
        const e = new Error()
        e.message = err.message
        e.stack = err.stack
        console.error(e)
      }
    }
    console.log('')
  }

  nicePath(fileName) {
    return path.relative(path.resolve(__dirname, '../..'), fileName)
  }

  niceResult(result) {
    switch (result) {
      case ResultType.PASS:
        return 'PASS'.green
      case ResultType.FAIL:
        return 'FAIL'.red
      case ResultType.SKIP:
        return 'SKIP'.cyan
    }
  }
}

module.exports = new TestReporter()