const path = require('path')
const childProcess = require('child_process')

const typeCheck = () => {
  const tscExec = path.resolve(require.resolve('typescript'), '../../bin/tsc')
  const tscChild = childProcess.spawn('node', [tscExec, '--project', '../../tsconfig.json'], {
    cwd: path.resolve(__dirname, 'test-smoke/electron')
  })
  tscChild.stdout.on('data', d => console.log(d.toString()))
  tscChild.stderr.on('data', d => console.error(d.toString()))
  tscChild.on('exit', (tscStatus) => {
    if (tscStatus !== 0) {
      process.exit(tscStatus)
    }
  })
}

typeCheck()
