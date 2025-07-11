import { expect } from 'chai';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as path from 'node:path';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('module resolution', () => {
  describe('electron condition', () => {
    it('resolves to electron-specific exports in main process and default exports in worker threads', async function () {
      const appPath = path.join(fixturesPath, 'api', 'module-resolve');
      const electronPath = process.execPath;
      const appProcess = cp.spawn(electronPath, [appPath]);

      const [code, signal] = await once(appProcess, 'exit');

      if (code !== 0) {
        let output = '';
        if (appProcess.stdout) {
          appProcess.stdout.on('data', (data) => { output += data; });
        }
        if (appProcess.stderr) {
          appProcess.stderr.on('data', (data) => { output += data; });
        }
        throw new Error(`Process exited with code ${code} and signal ${signal}. Output: ${output}`);
      }

      expect(code).to.equal(0);
      expect(signal).to.equal(null);
    });
  });
});
