const Module = require('module')
const path = require('path')
const v8 = require('v8')

Module.globalPaths.push(path.resolve(__dirname, '../spec/node_modules'))

// We want to terminate on errors, not throw up a dialog
process.on('uncaughtException', (err) => {
  console.error('Unhandled exception in main spec runner:', err)
  process.exit(1)
})

// Tell ts-node which tsconfig to use
process.env.TS_NODE_PROJECT = path.resolve(__dirname, '../tsconfig.spec.json')

const { app } = require('electron')

v8.setFlagsFromString('--expose_gc')
app.commandLine.appendSwitch('js-flags', '--expose_gc')
// Prevent the spec runner quiting when the first window closes
app.on('window-all-closed', () => null)
// TODO: This API should _probably_ only be enabled for the specific test that needs it
// not the entire test suite
app.commandLine.appendSwitch('ignore-certificate-errors')

app.whenReady().then(() => {
  require('ts-node/register')

  const argv = require('yargs')
    .boolean('ci')
    .string('g').alias('g', 'grep')
    .boolean('i').alias('i', 'invert')
    .argv

  const isCi = !!argv.ci
  global.isCI = isCi

  const Mocha = require('mocha')
  const mochaOptions = {}
  if (process.env.MOCHA_REPORTER) {
    mochaOptions.reporter = process.env.MOCHA_REPORTER
  }
  if (process.env.MOCHA_MULTI_REPORTERS) {
    mochaOptions.reporterOptions = {
      reporterEnabled: process.env.MOCHA_MULTI_REPORTERS
    }
  }
  const mocha = new Mocha(mochaOptions)

  if (!process.env.MOCHA_REPORTER) {
    mocha.ui('bdd').reporter('tap')
  }
  mocha.timeout(isCi ? 30000 : 10000)

  if (argv.grep) mocha.grep(argv.grep)
  if (argv.invert) mocha.invert()

  // Read all test files.
  const walker = require('walkdir').walk(__dirname, {
    no_recurse: true
  })

  // This allows you to run specific modules only:
  // npm run test -match=menu
  const moduleMatch = process.env.npm_config_match
    ? new RegExp(process.env.npm_config_match, 'g')
    : null

  walker.on('file', (file) => {
    if (/-spec\.[tj]s$/.test(file) &&
        (!moduleMatch || moduleMatch.test(file))) {
      mocha.addFile(file)
    }
  })

  walker.on('end', () => {
    const runner = mocha.run(() => {
      if (isCi && runner.hasOnly) {
        try {
          throw new Error('A spec contains a call to it.only or describe.only and should be reverted.')
        } catch (error) {
          console.error(error.stack || error)
        }
        process.exit(1)
      }

      process.exit(runner.failures)
    })
  })
})
