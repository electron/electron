import { expect } from 'chai';
import * as cp from 'child_process';
import * as fs from 'fs';
import * as path from 'path';
import { ifit } from './spec-helpers';

const fixturePath = path.resolve(__dirname, 'fixtures', 'crash-cases');

let children: cp.ChildProcessWithoutNullStreams[] = [];

const runFixtureAndEnsureCleanExit = (args: string[]) => {
  let out = '';
  const child = cp.spawn(process.execPath, args);
  children.push(child);
  child.stdout.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  child.stderr.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  return new Promise<void>((resolve) => {
    child.on('exit', (code, signal) => {
      if (code !== 0 || signal !== null) {
        console.error(out);
      }
      expect(signal).to.equal(null, 'exit signal should be null');
      expect(code).to.equal(0, 'should have exited with code 0');
      children = children.filter(c => c !== child);
      resolve();
    });
  });
};

describe('crash cases', () => {
  afterEach(() => {
    for (const child of children) {
      child.kill();
    }
    expect(children).to.have.lengthOf(0, 'all child processes should have exited cleanly');
    children.length = 0;
  });
  const cases = fs.readdirSync(fixturePath);

  for (const crashCase of cases) {
    // TODO(jkleinsc) fix this flaky test on Windows 32-bit
    ifit(process.platform !== 'win32' || process.arch !== 'ia32' || crashCase !== 'quit-on-crashed-event')(`the "${crashCase}" case should not crash`, () => {
      const fixture = path.resolve(fixturePath, crashCase);
      const argsFile = path.resolve(fixture, 'electron.args');
      const args = [fixture];
      if (fs.existsSync(argsFile)) {
        args.push(...fs.readFileSync(argsFile, 'utf8').trim().split('\n'));
      }
      return runFixtureAndEnsureCleanExit(args);
    });
  }
});
