import { app } from 'electron/main';

import { expect } from 'chai';

import * as childProcess from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import * as vm from 'node:vm';

import { ifdescribe } from './lib/spec-helpers';

// electron_xcache ships in xcache.zip next to dist.zip on Linux; CI unzips it
// beside the electron binary.
const xcache = path.resolve(process.execPath, '..', 'electron_xcache');

ifdescribe(process.platform === 'linux' && fs.existsSync(xcache))('electron_xcache', () => {
  let tmp: string;
  before(() => {
    tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-xcache-'));
  });
  after(() => {
    fs.rmSync(tmp, { recursive: true, force: true });
  });

  const generate = (source: string, ...args: string[]) => {
    const input = path.join(tmp, 'input.js');
    const output = path.join(tmp, 'output.cache');
    fs.writeFileSync(input, source);
    // The tool defaults to a plain main process's flags; the spec runner adds
    // its own --js-flags (spec/index.js), which are part of the cache key too.
    const result = childProcess.spawnSync(
      xcache,
      [
        '--snapshot',
        process.execPath,
        '--in',
        input,
        '--out',
        output,
        '--json',
        '--extra-v8-flags',
        app.commandLine.getSwitchValue('js-flags'),
        ...args
      ],
      { encoding: 'utf8' }
    );
    expect(result.status, result.stderr).to.equal(0);
    return { info: JSON.parse(result.stdout), cache: fs.readFileSync(output) };
  };

  it('finds the Node startup snapshot this process was created from', () => {
    const { info } = generate('1');
    expect(info.snapshot.kind).to.equal('node-startup-snapshot');
    const own = new vm.Script('1', { filename: 'probe' }).createCachedData();
    // SerializedCodeData header: [12] FlagList::Hash, [16] read-only snapshot checksum.
    expect(info.header.flagHash).to.equal('0x' + own.readUInt32LE(12).toString(16).padStart(8, '0'));
    expect(info.header.roSnapshotChecksum).to.equal('0x' + own.readUInt32LE(16).toString(16).padStart(8, '0'));
  });

  it('produces a script cache the main process accepts', () => {
    const source = 'function f (n) { return n * 7; } [f(6), typeof f];';
    const { cache } = generate(source, '--eager');
    const script = new vm.Script(source, { filename: 'xcache-script', cachedData: cache });
    expect(script.cachedDataRejected).to.equal(false);
    expect(script.runInThisContext()).to.deep.equal([42, 'function']);
  });

  it('produces a function cache the main process accepts', () => {
    const source = 'module.exports = { answer: exports.seed * 2, self: typeof require };';
    const { cache } = generate(source, '--mode', 'function');
    const fn = vm.compileFunction(source, ['exports', 'require', 'module', '__filename', '__dirname'], {
      filename: 'xcache-function',
      cachedData: cache
    }) as any;
    expect(fn.cachedDataRejected).to.equal(false);
    const mod = { exports: {} as any };
    fn({ seed: 21 }, require, mod, 'xcache-function', tmp);
    expect(mod.exports).to.deep.equal({ answer: 42, self: 'function' });
  });
});
