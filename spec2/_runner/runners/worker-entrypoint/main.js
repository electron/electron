const { app } = require('electron');

const { MessageKey, ResultType } = require('../constants')
const { getTestByPath } = require('../../define-helpers')
const { setCurrentFile } = require('../../shared')

const sendToRunner = (key, ...args)  => {
  process.send({
    key,
    args
  })
}

if (!('toJSON' in Error.prototype)) {
  Object.defineProperty(Error.prototype, 'toJSON', {
      value: function () {
          var alt = {}

          Object.getOwnPropertyNames(this).forEach(function (key) {
              alt[key] = this[key]
          }, this)

          return alt
      },
      configurable: true,
      writable: true
  })
}

app.on('ready', () => {
  sendToRunner(MessageKey.READY)
})

process.on('message', (message) => {
  switch (message.key) {
    case MessageKey.RUN_TEST:
      const runOpts = message.args[0]
      const sendTestResult = (result, err) => sendToRunner(MessageKey.TEST_RESULT, runOpts, result, err)

      setCurrentFile(runOpts.file)
      try {
        require(runOpts.file)
      } catch (err) {
        return sendTestResult(ResultType.FAIL, new Error(`Failed to load test file: ${runOpts.file}`))
      }
      const test = getTestByPath(runOpts.file, runOpts.testPath)
      if (!test) {
        return sendTestResult(ResultType.FAIL, new Error(`Failed to find test at given testPath: ${JSON.stringify(runOpts.testPath)}`))
      }
      test.run({
        registerPass: () => sendTestResult(ResultType.PASS),
        registerFail: (err) => { console.error('err', err); sendTestResult(ResultType.FAIL, err) },
        registerSkip: () => sendTestResult(ResultType.SKIP),
      }, runOpts.testPath)
      break;
  }
})
