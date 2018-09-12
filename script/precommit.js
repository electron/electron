#!/usr/bin/env node

const childProcess = require('child_process')
const path = require('path')

const SOURCE_ROOT = path.normalize(path.dirname(__dirname))

function callExecutable (executable, args) {
  console.log(executable + ' ' + args.join(' '))
  try {
    console.log(String(childProcess.execFileSync(executable, args, {cwd: SOURCE_ROOT})))
  } catch (e) {
    console.log(e)
    process.exit(1)
  }
}

const script = path.join(SOURCE_ROOT, 'script', 'run-clang-format.py')
const dirs = ['atom', 'brightray', 'chromium_src'].map(x => path.join(SOURCE_ROOT, x))
const args = [script, '-r', '-c', ...dirs]
callExecutable('python', args)

callExecutable(path.join(SOURCE_ROOT, 'script', 'cpplint.js'), ['--only-changed'])

callExecutable('npm', ['run', 'lint:js'])

callExecutable('remark', ['docs', '-qf'])
