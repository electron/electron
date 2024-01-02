import { expect } from 'chai';
import { startRemoteControlApp } from './lib/spec-helpers';
import { once } from 'node:events';
import { spawn } from 'node:child_process';
import { BrowserWindow } from 'electron';
import path = require('node:path');

describe('fuses', () => {
  it('can be enabled by command-line argument during testing', async () => {
    const child0 = spawn(process.execPath, ['-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code0] = await once(child0, 'exit');
    // Should exit with 9 because -e is not allowed in NODE_OPTIONS
    expect(code0).to.equal(9);
    const child1 = spawn(process.execPath, ['--set-fuse-node_options=0', '-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code1] = await once(child1, 'exit');
    // Should print the version and exit with 0
    expect(code1).to.equal(0);
  });

  it('disables fetching file:// URLs when grant_file_protocol_extra_privileges is 0', async () => {
    const rc = await startRemoteControlApp(['--set-fuse-grant_file_protocol_extra_privileges=0']);
    await expect(rc.remotely(async (fixture: string) => {
      const bw = new BrowserWindow({ show: false });
      await bw.loadFile(fixture);
      return await bw.webContents.executeJavaScript("ajax('file:///etc/passwd')");
    }, path.join(__dirname, 'fixtures', 'pages', 'fetch.html'))).to.eventually.be.rejectedWith('Failed to fetch');
  });
});
