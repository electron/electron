import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as path from 'node:path';

function spawnOomApp (scenario: string): Promise<{ stderr: string, code: number | null }> {
  const appPath = path.join(__dirname, 'fixtures', 'apps', 'oom-stack-trace');
  return new Promise((resolve) => {
    const child = cp.spawn(process.execPath, [appPath, `--scenario=${scenario}`]);
    let stderr = '';
    child.stderr.on('data', (chunk: Buffer) => {
      stderr += chunk.toString();
    });
    child.on('exit', (code) => {
      resolve({ stderr, code });
    });
  });
}

describe('OOM stack trace', () => {
  it('prints heap statistics to stderr when renderer approaches heap limit', async function () {
    this.timeout(30000);
    const { stderr } = await spawnOomApp('leak');
    expect(stderr).to.include('Near heap limit');
    expect(stderr).to.match(/Heap: used=[\d.]+MB limit=[\d.]+MB/);
  });

  it('captures JS function names in the stack trace', async function () {
    this.timeout(30000);
    const { stderr } = await spawnOomApp('leak');
    expect(stderr).to.include('leakMemory');
  });

  it('captures the calling function on JSON.stringify OOM (issue #46078)', async function () {
    this.timeout(30000);
    const { stderr } = await spawnOomApp('json');
    expect(stderr).to.include('Near heap limit');
    expect(stderr).to.include('serializeData');
  });
});
