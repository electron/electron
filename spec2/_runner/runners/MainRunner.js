const childProcess = require('child_process')
const path = require('path')

const BaseRunner = require('./Runner')
const { RunnerType, MessageKey } = require('./constants')
const testReporter = require('./TestReporter')

module.exports = class MainRunner extends BaseRunner {
  constructor(...args) {
    super(RunnerType.MAIN, ...args)
    this.d('launching child process')
    this.child = childProcess.spawn(
      path.resolve(__dirname, '../../../out/D/Electron.app/Contents/MacOS/Electron'),
      [path.resolve(__dirname, 'worker-entrypoint', 'main.js')],
      {
        cwd: path.resolve(__dirname, '../../..'),
        env: this.getChildEnv(),
        stdio: ['ipc']
      }
    )

    this.handleMessage = this.handleMessage.bind(this)

    this.child.on('message', this.handleMessage)
    this.child.stdout.on('data', (chunk) => this.d(chunk.toString()))
    this.child.stderr.on('data', (chunk) => this.d(chunk.toString()))

    process.on('exit', () => {
      if (!this.child.killed) {
        this.child.kill()
      }
    })
  }

  getChildEnv() {
    const env = Object.assign({}, process.env, {
      ELECTRON_ENABLE_STACK_DUMPING: 'true'
    })
    delete env.ELECTRON_ENABLE_LOGGING
    return env
  }

  handleMessage(message) {
    switch (message.key) {
      case MessageKey.READY:
        this.d('ready')
        this.pool.onRunnerReady(this)
        break;
      case MessageKey.TEST_RESULT:
        const [opts, result, err] = message.args
        this.d('got test result', result)
        testReporter.report(opts.file, opts.testPath, result, err)
        this.pool.onRunnerReady(this)
        break;
    }
  }

  sendMessage(key, ...args) {
    this.child.send({
      key,
      args
    })
  }

  runTest(file, testPath) {
    this.d(`running test: ${file}`, testPath)
    this.sendMessage(MessageKey.RUN_TEST, {
      file,
      testPath
    })
  }

  stop() {
    this.child.kill()
  }
}
