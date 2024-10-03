const childProcess = require('node:child_process');
const path = require('node:path');

const typeCheck = () => {
  const tscExec = path.resolve(require.resolve('typescript'), '../../bin/tsc');
  const tscChild = childProcess.spawn(process.execPath, [tscExec, '--project', './ts-smoke/tsconfig.json'], {
    cwd: path.resolve(__dirname, '../')
  });
  tscChild.stdout.on('data', d => console.log(d.toString()));
  tscChild.stderr.on('data', d => console.error(d.toString()));
  tscChild.on('exit', (tscStatus) => {
    if (tscStatus !== 0) {
      process.exit(tscStatus);
    }
  });
};

typeCheck();
