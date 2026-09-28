import * as childProcess from 'node:child_process';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';

const typeCheck = () => {
  const tscExec = path.resolve(fileURLToPath(import.meta.resolve('typescript/package.json')), '../bin/tsc');
  const tscChild = childProcess.spawn(process.execPath, [tscExec, '--project', './ts-smoke/tsconfig.json'], {
    cwd: path.resolve(import.meta.dirname, '../')
  });
  tscChild.stdout.on('data', (d) => console.log(d.toString()));
  tscChild.stderr.on('data', (d) => console.error(d.toString()));
  tscChild.on('exit', (tscStatus) => {
    if (tscStatus !== 0) {
      process.exit(tscStatus);
    }
  });
};

typeCheck();
