const TestSuite = require('./TestSuite')
const Test = require('./Test')
const { getTopLevelSuites } = require('./shared')

const describe = (name, describer) => {
  return new TestSuite(name, describer)
}

const it = (name, testFn) => {
  return new Test(name, testFn)
}

global.describe = describe
global.it = it

module.exports.getTestByPath = (fileName, testPath) => {
  let current = getTopLevelSuites().find(suite => suite.fileName === fileName && suite.name === testPath[0])
  for (const segment of testPath.slice(1, -1)) {
    if (!current) return null
    current = current.children.find(child => child.name === segment)
  }
  return current.tests.find(test => test.name === testPath[testPath.length - 1])
}